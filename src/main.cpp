#include "sdkconfig.h"

#include <alpaca_server/api.h>
#include <alpaca_server/device.h>
#include <alpaca_server/discovery.h>

#include <esp_app_desc.h>
#include <esp_http_server.h>
#include <esp_log.h>
#include <esp_ota_ops.h>
#include <nvs_flash.h>

#include "alpaca_switch.h"
#include "wifi_manager.h"
#include "switch_storage.h"
#include "ota_updater.h"
#include "alpaca_auth.h"

static const char *TAG = "main";

// WiFi configuration - update with your network credentials
#define WIFI_SSID "your_wifi_ssid"
#define WIFI_PASS "your_wifi_password"

// HTTP server handle
static httpd_handle_t server = NULL;

extern "C" void app_main(void)
{
    esp_log_level_set("*", ESP_LOG_INFO);
    
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP32 ASCOM Alpaca Switch Controller");
    
    // Initialize storage
    SwitchStorage::init();
    
    // Initialize authentication
    AlpacaAuth::init();
    
    // Get firmware version
    esp_app_desc_t desc;
    ESP_ERROR_CHECK(esp_ota_get_partition_description(esp_ota_get_running_partition(), &desc));
    ESP_LOGI(TAG, "Firmware version: %s", desc.version);
    
    // Initialize WiFi manager (this doesn't connect yet)
    WiFiManager& wifiManager = WiFiManager::getInstance();
    wifiManager.init(WIFI_SSID, WIFI_PASS);
    
    // Start WiFi connection process (non-blocking)
    wifiManager.connect();
    
    // Configure and start HTTP server
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 64;
    config.stack_size = 8192;
    config.lru_purge_enable = true;

    ESP_ERROR_CHECK(httpd_start(&server, &config));
    ESP_LOGI(TAG, "HTTP server started");
    
    // Create switch configurations
    switch_config_t switch_configs[5]; 
    
    // Initialize with default values
    for (int i = 0; i < 5; i++) {
        // Try to load from NVS first
        switch_storage_t saved_config;
        if (SwitchStorage::loadSwitch(i, &saved_config) == ESP_OK) {
            // Use saved configuration
            switch_configs[i].gpio_pin = saved_config.gpio_pin;
            switch_configs[i].normally_on = saved_config.normally_on;
            switch_configs[i].name = saved_config.name;
            switch_configs[i].description = saved_config.description;
            switch_configs[i].min_value = saved_config.min_value;
            switch_configs[i].max_value = saved_config.max_value;
            switch_configs[i].step = saved_config.step;
            switch_configs[i].can_write = saved_config.can_write;
        } else {
            // Set default values
            const int default_pins[5] = {12, 14, 27, 26, 25};
            
            switch_configs[i].gpio_pin = default_pins[i];
            switch_configs[i].normally_on = false;
            
            char name[32];
            snprintf(name, sizeof(name), "Switch %d", i);
            switch_configs[i].name = name;
            
            char desc[128];
            snprintf(desc, sizeof(desc), "GPIO Switch on pin %d", default_pins[i]);
            switch_configs[i].description = desc;
            
            switch_configs[i].min_value = 0.0;
            switch_configs[i].max_value = 1.0;
            switch_configs[i].step = 1.0;
            switch_configs[i].can_write = true;
        }
    }
    
    // Create ASCOM Switch device instance
    AlpacaSwitch* switchDevice = new AlpacaSwitch(switch_configs, 5);
    
    // Create vector of devices
    std::vector<AlpacaServer::Device *> devices;
    devices.push_back(switchDevice);
    
    // Create the Alpaca API server with our devices
    AlpacaServer::Api api(
        devices,
        "ESP32_SWITCH_SERIAL",
        "ESP32 Alpaca Switch Server",
        "YourName/Organization",
        desc.version,
        "Location Description"
    );
    
    // Register the API routes with the HTTP server
    api.register_routes(server);
    ESP_LOGI(TAG, "Alpaca API routes registered");

    // Start the Alpaca Discovery service - will work on local networks
    // even without internet connection
    ESP_LOGI(TAG, "Starting Alpaca Discovery server");
    alpaca_server_discovery_start(80);
    
    // Main loop
    ESP_LOGI(TAG, "System startup complete, entering main loop");
    
    // Try to load saved switch states
    bool states[5];
    double values[5];
    if (SwitchStorage::loadAllStates(states, values, 5) == ESP_OK) {
        // Apply saved states
        for (int i = 0; i < 5; i++) {
            // Only apply state if switch is writable
            if (switch_configs[i].can_write) {
                if (states[i]) {
                    switchDevice->put_setswitch(i, true);
                } else {
                    switchDevice->put_setswitchvalue(i, values[i]);
                }
            }
        }
        ESP_LOGI(TAG, "Loaded and applied saved switch states");
    }
    
    while (true) {
        // Check WiFi status
        bool connected = wifiManager.isConnected();
        
        // Periodically save switch states
        static int save_counter = 0;
        save_counter++;
        
        if (save_counter >= 30) {  // Every 30 seconds
            save_counter = 0;
            
            // Save all switch states
            bool states[5];
            double values[5];
            
            // Get current switch states
            for (int i = 0; i < 5; i++) {
                switchDevice->get_getswitch(i, &states[i]);
                switchDevice->get_getswitchvalue(i, &values[i]);
            }
            
            // Save to NVS
            SwitchStorage::saveAllStates(states, values, 5);
            ESP_LOGI(TAG, "Saved all switch states");
        }
        
        // Sleep for a while
        vTaskDelay(pdMS_TO_TICKS(1000));  // 1 second
    }
}