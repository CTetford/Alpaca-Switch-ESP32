#include "switch_storage.h"
#include <nvs_flash.h>
#include <nvs.h>
#include <esp_log.h>

static const char* TAG = "switch_storage";
const char* SwitchStorage::NVS_NAMESPACE = "switch_cfg";

esp_err_t SwitchStorage::init() {
    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS flash init failed: %s", esp_err_to_name(err));
    }
    
    return err;
}

esp_err_t SwitchStorage::saveSwitch(int id, const switch_storage_t* config) {
    nvs_handle_t handle;
    esp_err_t err;
    
    char key[16];
    snprintf(key, sizeof(key), "switch_%d", id);
    
    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS handle: %s", esp_err_to_name(err));
        return err;
    }
    
    err = nvs_set_blob(handle, key, config, sizeof(switch_storage_t));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write config blob: %s", esp_err_to_name(err));
    } else {
        err = nvs_commit(handle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to commit NVS: %s", esp_err_to_name(err));
        } else {
            ESP_LOGI(TAG, "Saved configuration for switch %d", id);
        }
    }
    
    nvs_close(handle);
    return err;
}

esp_err_t SwitchStorage::loadSwitch(int id, switch_storage_t* config) {
    nvs_handle_t handle;
    esp_err_t err;
    
    char key[16];
    snprintf(key, sizeof(key), "switch_%d", id);
    
    err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS handle: %s", esp_err_to_name(err));
        return err;
    }
    
    size_t required_size = sizeof(switch_storage_t);
    err = nvs_get_blob(handle, key, config, &required_size);
    if (err != ESP_OK) {
        if (err == ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGW(TAG, "No saved configuration for switch %d", id);
        } else {
            ESP_LOGE(TAG, "Failed to read config blob: %s", esp_err_to_name(err));
        }
    } else {
        ESP_LOGI(TAG, "Loaded configuration for switch %d", id);
    }
    
    nvs_close(handle);
    return err;
}

esp_err_t SwitchStorage::saveAllStates(const bool* states, const double* values, int count) {
    nvs_handle_t handle;
    esp_err_t err;
    
    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS handle: %s", esp_err_to_name(err));
        return err;
    }
    
    err = nvs_set_blob(handle, "states", states, count * sizeof(bool));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write states blob: %s", esp_err_to_name(err));
        nvs_close(handle);
        return err;
    }
    
    err = nvs_set_blob(handle, "values", values, count * sizeof(double));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write values blob: %s", esp_err_to_name(err));
        nvs_close(handle);
        return err;
    }
    
    err = nvs_commit(handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit NVS: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "Saved all switch states and values");
    }
    
    nvs_close(handle);
    return err;
}

esp_err_t SwitchStorage::loadAllStates(bool* states, double* values, int count) {
    nvs_handle_t handle;
    esp_err_t err;
    
    err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS handle: %s", esp_err_to_name(err));
        return err;
    }
    
    size_t required_size = count * sizeof(bool);
    err = nvs_get_blob(handle, "states", states, &required_size);
    if (err != ESP_OK) {
        if (err == ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGW(TAG, "No saved states found");
        } else {
            ESP_LOGE(TAG, "Failed to read states blob: %s", esp_err_to_name(err));
        }
        nvs_close(handle);
        return err;
    }
    
    required_size = count * sizeof(double);
    err = nvs_get_blob(handle, "values", values, &required_size);
    if (err != ESP_OK) {
        if (err == ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGW(TAG, "No saved values found");
        } else {
            ESP_LOGE(TAG, "Failed to read values blob: %s", esp_err_to_name(err));
        }
        nvs_close(handle);
        return err;
    }
    
    ESP_LOGI(TAG, "Loaded all switch states and values");
    
    nvs_close(handle);
    return ESP_OK;
}

esp_err_t SwitchStorage::clear() {
    nvs_handle_t handle;
    esp_err_t err;
    
    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS handle: %s", esp_err_to_name(err));
        return err;
    }
    
    err = nvs_erase_all(handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to erase NVS namespace: %s", esp_err_to_name(err));
    } else {
        err = nvs_commit(handle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to commit NVS: %s", esp_err_to_name(err));
        } else {
            ESP_LOGI(TAG, "Cleared all switch configurations");
        }
    }
    
    nvs_close(handle);
    return err;
}