// wifi_manager.h
#pragma once

#include <esp_err.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>

// Public event group bits
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_DISCONNECTED_BIT BIT1
#define WIFI_SCANNING_BIT BIT2

class WiFiManager {
public:
    // Singleton instance getter
    static WiFiManager& getInstance() {
        static WiFiManager instance;
        return instance;
    }
    
    // Initialize WiFi manager
    esp_err_t init(const char* ssid, const char* password);
    
    // Start WiFi connection process
    esp_err_t connect();
    
    // Disconnect from WiFi
    esp_err_t disconnect();
    
    // Get current connection status
    bool isConnected();
    
    // Get connection event group handle
    EventGroupHandle_t getEventGroup() { return _eventGroup; }
    
    // Get IP address as string
    void getIpAddressStr(char* buffer, size_t buffer_size);

private:
    // Private constructor for singleton
    WiFiManager();
    ~WiFiManager();
    
    // Prevent copy/move construction and assignment
    WiFiManager(const WiFiManager&) = delete;
    WiFiManager& operator=(const WiFiManager&) = delete;
    WiFiManager(WiFiManager&&) = delete;
    WiFiManager& operator=(WiFiManager&&) = delete;
    
    // Connection task
    static void connectionTask(void* arg);
    
    // Connection parameters
    char _ssid[33];
    char _password[65];
    
    // Task handle
    TaskHandle_t _taskHandle;
    
    // Event group for status signaling
    EventGroupHandle_t _eventGroup;
    
    // Connection status
    bool _initialized;
};