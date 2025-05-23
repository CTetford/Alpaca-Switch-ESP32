#include "sdkconfig.h"

#include <alpaca_server/api.h>
#include <alpaca_server/device.h>
#include <alpaca_server/discovery.h>

#include <esp_app_desc.h>
#include <esp_http_server.h>
#include <esp_log.h>
#include <esp_ota_ops.h>
#include <nvs_flash.h>
#include <esp_netif.h>
#include <esp_wifi.h>
#include <string> // Added for std::string
#include <esp_efuse.h> // Added for MAC address

#include "alpaca_switch.h"
#include "wifi_manager.h"
#include "switch_storage.h"
#include "ota_updater.h"
#include "alpaca_auth.h"
#include "config.h"

static const char *TAG = "main";

// HTTP Server Configuration
#define HTTP_SERVER_MAX_URI_HANDLERS 64
#define HTTP_SERVER_STACK_SIZE 8192

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
    
    // Initialize WiFi with configuration
    wifiManager.init(WIFI_SSID, WIFI_PASS);
    
    // Configure static IP if enabled
    #ifdef USE_STATIC_IP
        esp_netif_ip_info_t ip_info;
        ip_info.ip.addr = ipaddr_addr(STATIC_IP);
        ip_info.gw.addr = ipaddr_addr(STATIC_GATEWAY);
        ip_info.netmask.addr = ipaddr_addr(STATIC_NETMASK);
        
        esp_netif_dns_info_t dns_info;
        dns_info.ip.u_addr.ip4.addr = ipaddr_addr(STATIC_DNS1);
        dns_info.ip.type = IPADDR_TYPE_V4;
        
        wifiManager.setStaticIP(ip_info, dns_info);
        ESP_LOGI(TAG, "Static IP configuration enabled: %s", STATIC_IP);
    #endif
    
    // Start WiFi connection process (non-blocking)
    wifiManager.connect();
    
    // Configure and start HTTP server
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = HTTP_SERVER_MAX_URI_HANDLERS;
    config.stack_size = HTTP_SERVER_STACK_SIZE;
    config.lru_purge_enable = true;

    ESP_ERROR_CHECK(httpd_start(&server, &config));
    ESP_LOGI(TAG, "HTTP server started");
    
    // Create switch configurations
    switch_config_t switch_configs[DEFAULT_NUM_SWITCHES]; 
    
    // Initialize with default values
    for (int i = 0; i < DEFAULT_NUM_SWITCHES; i++) {
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
            switch_configs[i].gpio_pin = DEFAULT_SWITCH_PINS[i];
            switch_configs[i].normally_on = DEFAULT_SWITCH_NORMAL_STATES[i];
            
            char name[32];
            snprintf(name, sizeof(name), "Switch %d", i);
            switch_configs[i].name = name;
            
            char desc[128];
            snprintf(desc, sizeof(desc), "GPIO Switch on pin %d", DEFAULT_SWITCH_PINS[i]);
            switch_configs[i].description = desc;
            
            switch_configs[i].min_value = DEFAULT_SWITCH_MIN_VALUES[i];
            switch_configs[i].max_value = DEFAULT_SWITCH_MAX_VALUES[i];
            switch_configs[i].step = DEFAULT_SWITCH_STEPS[i];
            switch_configs[i].can_write = DEFAULT_SWITCH_CAN_WRITE[i];
        }
    }
    
    // Create ASCOM Switch device instance
    AlpacaSwitch* switchDevice = new AlpacaSwitch(switch_configs, DEFAULT_NUM_SWITCHES, desc.version);
    
    // Create vector of devices
    std::vector<AlpacaServer::Device *> devices;
    devices.push_back(switchDevice);
    
    // Create the Alpaca API server with our devices
    const char* default_serial = "ESP32_SWITCH_SERIAL";
    std::string unique_id_str;

    if (strcmp(DEVICE_SERIAL, default_serial) == 0) {
        uint8_t mac[6];
        char mac_str[13]; // 12 chars for MAC, 1 for null terminator
        // Get MAC address from EFUSE
        esp_efuse_mac_get_default(mac); 
        sprintf(mac_str, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        unique_id_str = std::string(DEVICE_NAME) + "-" + mac_str;
        ESP_LOGI(TAG, "Using generated UniqueID: %s", unique_id_str.c_str());
    } else {
        unique_id_str = DEVICE_SERIAL;
        ESP_LOGI(TAG, "Using UniqueID from config: %s", unique_id_str.c_str());
    }

    AlpacaServer::Api api(
        devices,
        unique_id_str.c_str(), // Use generated or configured UniqueID
        DEVICE_NAME,
        DEVICE_ORGANIZATION,
        desc.version,
        DEVICE_LOCATION
    );
    
    // Register the API routes with the HTTP server
    api.register_routes(server);
    ESP_LOGI(TAG, "Alpaca API routes registered");

    // Start the Alpaca Discovery service - will work on local networks
    // even without internet connection
    ESP_LOGI(TAG, "Starting Alpaca Discovery server");
    alpaca_server_discovery_start(HTTP_SERVER_PORT);
    
    // Main loop
    ESP_LOGI(TAG, "System startup complete, entering main loop");
    
    while (true) {
        // Check WiFi status
        bool connected = wifiManager.isConnected();
        
        // Sleep for a while
        vTaskDelay(pdMS_TO_TICKS(1000));  // 1 second
    }
}