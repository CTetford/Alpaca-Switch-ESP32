#include "alpaca_switch.h"
#include <string.h>
#include <esp_log.h>

static const char* TAG = "alpaca_switch";

// Constructor with configurable switches
AlpacaSwitch::AlpacaSwitch(const switch_config_t* configs, int num_switches, const char* firmware_version) : Switch()
{
    _connected = true; // Start as connected regardless of WiFi
    _num_switches = num_switches;
    _firmware_version = firmware_version;
    
    // Allocate memory for switch properties
    _switch_states = new bool[num_switches];
    _switch_names = new char*[num_switches];
    _switch_descriptions = new char*[num_switches];
    _switch_values = new double[num_switches];
    _switch_can_write = new bool[num_switches];
    _min_switch_values = new double[num_switches];
    _max_switch_values = new double[num_switches];
    _switch_steps = new double[num_switches];
    _switch_pins = new int[num_switches];
    
    // Initialize the switches with values from config
    for (int i = 0; i < num_switches; i++) {
        _switch_states[i] = configs[i].normally_on;
        _switch_values[i] = configs[i].normally_on ? 1.0 : 0.0;
        
        // Allocate and copy strings
        _switch_names[i] = new char[32];
        snprintf(_switch_names[i], 32, "%s", configs[i].name ? configs[i].name : "Switch");
        
        _switch_descriptions[i] = new char[128];
        snprintf(_switch_descriptions[i], 128, "%s", configs[i].description ? configs[i].description : "GPIO Switch");
        
        _switch_can_write[i] = configs[i].can_write;
        _min_switch_values[i] = configs[i].min_value;
        _max_switch_values[i] = configs[i].max_value;
        _switch_steps[i] = configs[i].step;
        _switch_pins[i] = configs[i].gpio_pin;
        
        // Configure GPIO pins
        if (_switch_pins[i] >= 0) {
            gpio_reset_pin((gpio_num_t)_switch_pins[i]);
            gpio_set_direction((gpio_num_t)_switch_pins[i], GPIO_MODE_OUTPUT);
            gpio_set_level((gpio_num_t)_switch_pins[i], _switch_states[i] ? 1 : 0);
            ESP_LOGI(TAG, "Initialized switch %d on GPIO %d, initial state: %s", 
                    i, _switch_pins[i], _switch_states[i] ? "ON" : "OFF");
        } else {
            ESP_LOGI(TAG, "Initialized virtual switch %d, initial state: %s", 
                    i, _switch_states[i] ? "ON" : "OFF");
        }
    }
}

AlpacaSwitch::~AlpacaSwitch()
{
    // Free allocated memory
    for (int i = 0; i < _num_switches; i++) {
        delete[] _switch_names[i];
        delete[] _switch_descriptions[i];
    }
    
    delete[] _switch_states;
    delete[] _switch_names;
    delete[] _switch_descriptions;
    delete[] _switch_values;
    delete[] _switch_can_write;
    delete[] _min_switch_values;
    delete[] _max_switch_values;
    delete[] _switch_steps;
    delete[] _switch_pins;
}

// Common device interface methods

esp_err_t AlpacaSwitch::action(const char *action, const char *parameters, char *buf, size_t len)
{
    // No custom actions implemented
    return ALPACA_ERR_ACTION_NOT_IMPLEMENTED;
}

esp_err_t AlpacaSwitch::commandblind(const char *command, bool raw)
{
    // No blind commands implemented
    return ALPACA_ERR_ACTION_NOT_IMPLEMENTED;
}

esp_err_t AlpacaSwitch::commandbool(const char *command, bool raw, bool *resp)
{
    // No boolean commands implemented
    return ALPACA_ERR_ACTION_NOT_IMPLEMENTED;
}

esp_err_t AlpacaSwitch::commandstring(const char *action, bool raw, char *buf, size_t len)
{
    // No string commands implemented
    return ALPACA_ERR_ACTION_NOT_IMPLEMENTED;
}

esp_err_t AlpacaSwitch::get_connected(bool *connected)
{
    *connected = _connected;
    return ALPACA_OK;
}

esp_err_t AlpacaSwitch::set_connected(bool connected)
{
    _connected = connected;
    ESP_LOGI(TAG, "Switch connection state set to: %s", connected ? "connected" : "disconnected");
    return ALPACA_OK;
}

esp_err_t AlpacaSwitch::get_description(char *buf, size_t len)
{
    strncpy(buf, "ESP32 ASCOM Alpaca Switch Controller", len);
    return ALPACA_OK;
}

esp_err_t AlpacaSwitch::get_driverinfo(char *buf, size_t len)
{
    strncpy(buf, "ESP32 ASCOM Alpaca Switch Implementation", len);
    return ALPACA_OK;
}

esp_err_t AlpacaSwitch::get_driverversion(char *buf, size_t len)
{
    strncpy(buf, _firmware_version.c_str(), len);
    return ALPACA_OK;
}

esp_err_t AlpacaSwitch::get_interfaceversion(uint32_t *version)
{
    *version = 1; // As per requirements
    return ALPACA_OK;
}

esp_err_t AlpacaSwitch::get_name(char *buf, size_t len)
{
    strncpy(buf, "ESP32 Switch Controller", len);
    return ALPACA_OK;
}

esp_err_t AlpacaSwitch::get_supportedactions(std::vector<std::string> &actions)
{
    actions.clear();
    return ALPACA_OK;
}

// Switch specific interface methods

esp_err_t AlpacaSwitch::get_maxswitch(int32_t *maxswitch)
{
    *maxswitch = _num_switches;
    return ALPACA_OK;
}

esp_err_t AlpacaSwitch::get_canwrite(int32_t id, bool *canwrite)
{
    if (id < 0 || id >= _num_switches) {
        ESP_LOGW(TAG, "Invalid switch ID: %ld", id);
        return ALPACA_ERR_INVALID_VALUE;
    }
    
    *canwrite = _switch_can_write[id];
    return ALPACA_OK;
}

esp_err_t AlpacaSwitch::get_getswitch(int32_t id, bool *getswitch)
{
    if (id < 0 || id >= _num_switches) {
        ESP_LOGW(TAG, "Invalid switch ID: %ld", id);
        return ALPACA_ERR_INVALID_VALUE;
    }
    
    *getswitch = _switch_states[id];
    ESP_LOGD(TAG, "Get switch %ld state: %s", id, _switch_states[id] ? "ON" : "OFF");
    return ALPACA_OK;
}

esp_err_t AlpacaSwitch::get_getswitchdescription(int32_t id, char *buf, size_t len)
{
    if (id < 0 || id >= _num_switches) {
        ESP_LOGW(TAG, "Invalid switch ID: %ld", id);
        return ALPACA_ERR_INVALID_VALUE;
    }
    
    strncpy(buf, _switch_descriptions[id], len);
    return ALPACA_OK;
}

esp_err_t AlpacaSwitch::get_getswitchname(int32_t id, char *buf, size_t len)
{
    if (id < 0 || id >= _num_switches) {
        ESP_LOGW(TAG, "Invalid switch ID: %ld", id);
        return ALPACA_ERR_INVALID_VALUE;
    }
    
    strncpy(buf, _switch_names[id], len);
    return ALPACA_OK;
}

esp_err_t AlpacaSwitch::get_getswitchvalue(int32_t id, double *value)
{
    if (id < 0 || id >= _num_switches) {
        ESP_LOGW(TAG, "Invalid switch ID: %ld", id);
        return ALPACA_ERR_INVALID_VALUE;
    }
    
    *value = _switch_values[id];
    ESP_LOGD(TAG, "Get switch %ld value: %f", id, _switch_values[id]);
    return ALPACA_OK;
}

esp_err_t AlpacaSwitch::get_minswitchvalue(int32_t id, double *value)
{
    if (id < 0 || id >= _num_switches) {
        ESP_LOGW(TAG, "Invalid switch ID: %ld", id);
        return ALPACA_ERR_INVALID_VALUE;
    }
    
    *value = _min_switch_values[id];
    return ALPACA_OK;
}

esp_err_t AlpacaSwitch::get_maxswitchvalue(int32_t id, double *value)
{
    if (id < 0 || id >= _num_switches) {
        ESP_LOGW(TAG, "Invalid switch ID: %ld", id);
        return ALPACA_ERR_INVALID_VALUE;
    }
    
    *value = _max_switch_values[id];
    return ALPACA_OK;
}

esp_err_t AlpacaSwitch::put_setswitch(int32_t id, bool value)
{
    if (id < 0 || id >= _num_switches) {
        ESP_LOGW(TAG, "Invalid switch ID: %ld", id);
        return ALPACA_ERR_INVALID_VALUE;
    }
    
    if (!_connected) {
        ESP_LOGW(TAG, "Cannot set switch %ld - device not connected", id);
        return ALPACA_ERR_NOT_CONNECTED;
    }
    
    if (!_switch_can_write[id]) {
        ESP_LOGW(TAG, "Cannot set switch %ld - switch is read-only", id);
        return ALPACA_ERR_INVALID_OPERATION;
    }
    
    // Set the switch state
    _switch_states[id] = value;
    
    // Update the switch value
    _switch_values[id] = value ? 1.0 : 0.0;
    
    // Set the physical GPIO pin if valid
    if (_switch_pins[id] >= 0) {
        gpio_set_level((gpio_num_t)_switch_pins[id], value ? 1 : 0);
    }
    
    ESP_LOGI(TAG, "Switch %ld set to %s", id, value ? "ON" : "OFF");
    return ALPACA_OK;
}

esp_err_t AlpacaSwitch::put_setswitchname(int32_t id, const char *name)
{
    if (id < 0 || id >= _num_switches) {
        ESP_LOGW(TAG, "Invalid switch ID: %ld", id);
        return ALPACA_ERR_INVALID_VALUE;
    }
    
    strncpy(_switch_names[id], name, 31);
    _switch_names[id][31] = '\0'; // Ensure null termination
    
    ESP_LOGI(TAG, "Switch %ld name set to: %s", id, name);
    return ALPACA_OK;
}

esp_err_t AlpacaSwitch::put_setswitchvalue(int32_t id, double value)
{
    if (id < 0 || id >= _num_switches) {
        ESP_LOGW(TAG, "Invalid switch ID: %ld", id);
        return ALPACA_ERR_INVALID_VALUE;
    }
    
    if (!_connected) {
        ESP_LOGW(TAG, "Cannot set switch %ld value - device not connected", id);
        return ALPACA_ERR_NOT_CONNECTED;
    }
    
    if (!_switch_can_write[id]) {
        ESP_LOGW(TAG, "Cannot set switch %ld value - switch is read-only", id);
        return ALPACA_ERR_INVALID_OPERATION;
    }
    
    // Check if the value is within range
    if (value < _min_switch_values[id] || value > _max_switch_values[id]) {
        ESP_LOGW(TAG, "Invalid switch %ld value: %f (range: %f to %f)", 
                 id, value, _min_switch_values[id], _max_switch_values[id]);
        return ALPACA_ERR_INVALID_VALUE;
    }
    
    // Set the switch value
    _switch_values[id] = value;
    
    // Update the switch state (on if value > 0)
    bool new_state = value > 0;
    _switch_states[id] = new_state;
    
    // Set the physical GPIO pin if valid
    if (_switch_pins[id] >= 0) {
        gpio_set_level((gpio_num_t)_switch_pins[id], new_state ? 1 : 0);
    }
    
    ESP_LOGI(TAG, "Switch %ld value set to %f (state: %s)", id, value, new_state ? "ON" : "OFF");
    return ALPACA_OK;
}

esp_err_t AlpacaSwitch::get_switchstep(int32_t id, double *switchstep)
{
    if (id < 0 || id >= _num_switches) {
        ESP_LOGW(TAG, "Invalid switch ID: %ld", id);
        return ALPACA_ERR_INVALID_VALUE;
    }
    
    *switchstep = _switch_steps[id];
    return ALPACA_OK;
}