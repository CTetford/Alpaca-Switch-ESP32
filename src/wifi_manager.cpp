// wifi_manager.cpp
#include "wifi_manager.h"
#include <string.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_netif.h>
#include <lwip/ip4_addr.h>

static const char* TAG = "wifi_manager";

// Event handler for WiFi events
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    WiFiManager* manager = static_cast<WiFiManager*>(arg);
    EventGroupHandle_t eventGroup = manager->getEventGroup();
    
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                ESP_LOGI(TAG, "WiFi station started");
                break;
                
            case WIFI_EVENT_STA_CONNECTED:
                ESP_LOGI(TAG, "Connected to access point");
                break;
                
            case WIFI_EVENT_STA_DISCONNECTED:
                ESP_LOGW(TAG, "Disconnected from access point");
                
                // Clear connected bit, set disconnected bit
                xEventGroupClearBits(eventGroup, WIFI_CONNECTED_BIT);
                xEventGroupSetBits(eventGroup, WIFI_DISCONNECTED_BIT);
                break;
                
            default:
                break;
        }
    } else if (event_base == IP_EVENT) {
        if (event_id == IP_EVENT_STA_GOT_IP) {
            ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
            ESP_LOGI(TAG, "Got IP address: " IPSTR, IP2STR(&event->ip_info.ip));
            
            // Set connected bit, clear disconnected bit
            xEventGroupSetBits(eventGroup, WIFI_CONNECTED_BIT);
            xEventGroupClearBits(eventGroup, WIFI_DISCONNECTED_BIT);
        }
    }
}

WiFiManager::WiFiManager()
    : _taskHandle(NULL), _initialized(false), _netif(NULL)
{
    _eventGroup = xEventGroupCreate();
    xEventGroupSetBits(_eventGroup, WIFI_DISCONNECTED_BIT);
}

WiFiManager::~WiFiManager()
{
    // Cleanup
    if (_taskHandle != NULL) {
        vTaskDelete(_taskHandle);
        _taskHandle = NULL;
    }
    
    if (_eventGroup != NULL) {
        vEventGroupDelete(_eventGroup);
        _eventGroup = NULL;
    }
}

esp_err_t WiFiManager::init(const char* ssid, const char* password)
{
    if (_initialized) {
        ESP_LOGW(TAG, "WiFi manager already initialized");
        return ESP_OK;
    }
    
    // Copy credentials
    strncpy(_ssid, ssid, sizeof(_ssid) - 1);
    _ssid[sizeof(_ssid) - 1] = '\0';
    
    strncpy(_password, password, sizeof(_password) - 1);
    _password[sizeof(_password) - 1] = '\0';
    
    // Initialize networking components
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    _netif = esp_netif_create_default_wifi_sta();
    
    // Initialize WiFi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    // Register event handlers
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, this));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, this));
    
    // Configure WiFi station
    wifi_config_t wifi_config = {};
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;
    strncpy((char*)wifi_config.sta.ssid, _ssid, sizeof(wifi_config.sta.ssid));
    strncpy((char*)wifi_config.sta.password, _password, sizeof(wifi_config.sta.password));
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    
    _initialized = true;
    return ESP_OK;
}

esp_err_t WiFiManager::connect()
{
    if (!_initialized) {
        ESP_LOGE(TAG, "WiFi manager not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    // Start WiFi
    ESP_ERROR_CHECK(esp_wifi_start());
    
    // Connect to access point
    ESP_ERROR_CHECK(esp_wifi_connect());
    
    ESP_LOGI(TAG, "WiFi connection started");
    return ESP_OK;
}

bool WiFiManager::isConnected()
{
    if (!_initialized) {
        return false;
    }
    
    return (xEventGroupGetBits(_eventGroup) & WIFI_CONNECTED_BIT) != 0;
}

esp_err_t WiFiManager::setStaticIP(const esp_netif_ip_info_t& ip_info, esp_netif_dns_info_t& dns_info)
{
    if (!_initialized) {
        ESP_LOGE(TAG, "WiFi manager not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    if (_netif == NULL) {
        ESP_LOGE(TAG, "Network interface not created");
        return ESP_ERR_INVALID_STATE;
    }
    
    // Set static IP configuration
    ESP_ERROR_CHECK(esp_netif_dhcpc_stop(_netif));
    ESP_ERROR_CHECK(esp_netif_set_ip_info(_netif, &ip_info));
    
    // Set DNS configuration
    ESP_ERROR_CHECK(esp_netif_set_dns_info(_netif, ESP_NETIF_DNS_MAIN, &dns_info));
    
    ESP_LOGI(TAG, "Static IP configuration set");
    return ESP_OK;
}

esp_err_t WiFiManager::disconnect()
{
    return esp_wifi_disconnect();
}

void WiFiManager::getIpAddressStr(char* buffer, size_t buffer_size)
{
    if (!_initialized || _netif == NULL) {
        strncpy(buffer, "0.0.0.0", buffer_size);
        return;
    }
    
    esp_netif_ip_info_t ip_info;
    if (esp_netif_get_ip_info(_netif, &ip_info) != ESP_OK) {
        strncpy(buffer, "0.0.0.0", buffer_size);
        return;
    }
    
    snprintf(buffer, buffer_size, IPSTR, IP2STR(&ip_info.ip));
}

void WiFiManager::connectionTask(void* arg)
{
    WiFiManager* manager = static_cast<WiFiManager*>(arg);
    EventGroupHandle_t eventGroup = manager->getEventGroup();
    
    const int MAX_RETRY_COUNT = 10;
    int retry_count = 0;
    int reconnect_delay_ms = 5000; // Start with 5 seconds
    
    while (true) {
        EventBits_t bits = xEventGroupGetBits(eventGroup);
        
        // If disconnected, try to reconnect
        if ((bits & WIFI_DISCONNECTED_BIT) && !(bits & WIFI_CONNECTED_BIT)) {
            // Set scanning bit
            xEventGroupSetBits(eventGroup, WIFI_SCANNING_BIT);
            
            ESP_LOGI(TAG, "Attempting to connect to WiFi (%d)...", retry_count + 1);
            esp_wifi_connect();
            
            // Wait for connect/disconnect event
            bits = xEventGroupWaitBits(eventGroup,
                                       WIFI_CONNECTED_BIT | WIFI_DISCONNECTED_BIT,
                                       pdFALSE,
                                       pdFALSE,
                                       pdMS_TO_TICKS(10000)); // 10 second timeout
            
            // Clear scanning bit
            xEventGroupClearBits(eventGroup, WIFI_SCANNING_BIT);
            
            if (bits & WIFI_CONNECTED_BIT) {
                ESP_LOGI(TAG, "WiFi connected successfully");
                retry_count = 0;
                reconnect_delay_ms = 5000; // Reset delay on success
            } else {
                retry_count++;
                
                // If we've tried too many times, back off for longer
                if (retry_count > MAX_RETRY_COUNT) {
                    reconnect_delay_ms = 30000; // 30 seconds
                    ESP_LOGW(TAG, "Multiple connection failures, backing off for %d seconds", 
                             reconnect_delay_ms / 1000);
                    retry_count = 0;
                }
                
                ESP_LOGW(TAG, "Failed to connect to WiFi, will retry in %d seconds", 
                         reconnect_delay_ms / 1000);
                vTaskDelay(pdMS_TO_TICKS(reconnect_delay_ms));
            }
        } else {
            // Already connected, just wait for a disconnect event
            xEventGroupWaitBits(eventGroup,
                               WIFI_DISCONNECTED_BIT,
                               pdFALSE,
                               pdFALSE,
                               pdMS_TO_TICKS(5000)); // 5 second polling
        }
    }
}