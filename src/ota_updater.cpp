#include "ota_updater.h"
#include <esp_ota_ops.h>
#include <esp_http_client.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char* TAG = "ota_updater";

std::string OtaUpdater::_firmware_url;
bool OtaUpdater::_updateInProgress = false;
int OtaUpdater::_updateProgress = 0;
std::string OtaUpdater::_lastStatusMessage = "No update attempted";

#define BUFFER_SIZE 1024

// Task handle for the update process
static TaskHandle_t update_task_handle = NULL;

esp_err_t OtaUpdater::startUpdate(const std::string& url) {
    if (_updateInProgress) {
        _lastStatusMessage = "Update already in progress";
        ESP_LOGW(TAG, "Update already in progress");
        return ESP_ERR_INVALID_STATE;
    }
    
    // Reset progress and status
    _updateProgress = 0;
    _updateInProgress = true;
    _lastStatusMessage = "Update started";
    
    // Store the URL for the update task
    _firmware_url = url;
    
    // Start update task
    if (xTaskCreate(updateTask, "ota_update", 8192, NULL, 5, &update_task_handle) != pdPASS) {
        _updateInProgress = false;
        _lastStatusMessage = "Failed to start update task";
        ESP_LOGE(TAG, "Failed to create update task");
        return ESP_ERR_NO_MEM;
    }
    
    return ESP_OK;
}

bool OtaUpdater::isUpdateInProgress() {
    return _updateInProgress;
}

int OtaUpdater::getUpdateProgress() {
    return _updateProgress;
}

std::string OtaUpdater::getLastStatusMessage() {
    return _lastStatusMessage;
}

std::string OtaUpdater::getFirmwareVersion() {
    esp_app_desc_t app_desc;
    esp_ota_get_partition_description(esp_ota_get_running_partition(), &app_desc);
    return std::string(app_desc.version);
}

void OtaUpdater::updateTask(void* pvParameter) {
    esp_err_t err;
    esp_http_client_config_t config = {};
    esp_http_client_handle_t client = NULL;
    esp_ota_handle_t update_handle = 0;
    const esp_partition_t *update_partition = NULL;
    int content_length = 0;
    int binary_file_length = 0;
    int bytes_read = 0;
    char *buffer = NULL;

    config.url = _firmware_url.c_str();
    config.timeout_ms = 5000;
    config.skip_cert_common_name_check = true;

    ESP_LOGI(TAG, "Starting OTA update from: %s", config.url);

    client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        goto cleanup;
    }

    err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        goto cleanup;
    }

    content_length = esp_http_client_fetch_headers(client);
    if (content_length < 0) {
        ESP_LOGE(TAG, "Failed to get content length");
        goto cleanup;
    }

    update_partition = esp_ota_get_next_update_partition(NULL);
    if (update_partition == NULL) {
        ESP_LOGE(TAG, "Failed to get update partition");
        goto cleanup;
    }

    ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%x",
             update_partition->subtype, (unsigned int)update_partition->address);

    err = esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &update_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
        goto cleanup;
    }

    buffer = (char *)malloc(BUFFER_SIZE);
    if (!buffer) {
        ESP_LOGE(TAG, "Failed to allocate buffer");
        goto cleanup;
    }

    while (1) {
        bytes_read = esp_http_client_read(client, buffer, BUFFER_SIZE);
        if (bytes_read < 0) {
            ESP_LOGE(TAG, "Failed to read data");
            goto cleanup;
        }
        else if (bytes_read == 0) {
            break;
        }

        err = esp_ota_write(update_handle, buffer, bytes_read);
        if (err != ESP_OK) {
            goto cleanup;
        }

        binary_file_length += bytes_read;
        
        // Update progress (0-100%)
        if (content_length > 0) {
            _updateProgress = (binary_file_length * 100) / content_length;
            _lastStatusMessage = "Downloading firmware: " + std::to_string(_updateProgress) + "%";
        }
        
        ESP_LOGD(TAG, "Written image length %d", binary_file_length);
    }

    if (esp_ota_end(update_handle) != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_end failed!");
        goto cleanup;
    }

    if (esp_ota_set_boot_partition(update_partition) != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed!");
        goto cleanup;
    }

    _lastStatusMessage = "Update successful! Rebooting...";
    ESP_LOGI(TAG, "Update successful! Rebooting...");
    esp_restart();

cleanup:
    if (update_handle) {
        esp_ota_end(update_handle);
    }
    if (buffer) {
        free(buffer);
    }
    if (client) {
        esp_http_client_cleanup(client);
    }
    _updateInProgress = false;
    vTaskDelete(NULL);
}