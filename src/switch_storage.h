#pragma once

#include <esp_err.h>
#include <stdint.h>
#include <stdbool.h>

// Structure for storing switch configuration
typedef struct {
    char name[32];
    char description[128];
    double min_value;
    double max_value;
    double step;
    bool can_write;
    bool normally_on;
    int gpio_pin;
    bool state;
    double value;
} switch_storage_t;

class SwitchStorage {
public:
    // Initialize storage
    static esp_err_t init();
    
    // Save switch configuration
    static esp_err_t saveSwitch(int id, const switch_storage_t* config);
    
    // Load switch configuration
    static esp_err_t loadSwitch(int id, switch_storage_t* config);
    
    // Save all switch states
    static esp_err_t saveAllStates(const bool* states, const double* values, int count);
    
    // Load all switch states
    static esp_err_t loadAllStates(bool* states, double* values, int count);
    
    // Clear all storage
    static esp_err_t clear();
    
private:
    static const char* NVS_NAMESPACE;
};