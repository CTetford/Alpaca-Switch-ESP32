#pragma once
#ifndef ALPACA_SWITCH_H
#define ALPACA_SWITCH_H

#include <alpaca_server/api.h>
#include <driver/gpio.h>

// Switch configuration struct
typedef struct {
    int gpio_pin;           // GPIO pin number
    bool normally_on;       // Initial state (true = on at boot)
    const char* name;       // Default name
    const char* description; // Default description
    double min_value;       // Minimum value
    double max_value;       // Maximum value
    double step;            // Step value
    bool can_write;         // Whether the switch can be modified
} switch_config_t;

class AlpacaSwitch : public AlpacaServer::Switch
{
public:
    AlpacaSwitch(const switch_config_t* configs, int num_switches);
    ~AlpacaSwitch();

public:
    // Common device interface
    virtual esp_err_t action(const char *action, const char *parameters, char *buf, size_t len) override;
    virtual esp_err_t commandblind(const char *command, bool raw) override;
    virtual esp_err_t commandbool(const char *command, bool raw, bool *resp) override;
    virtual esp_err_t commandstring(const char *action, bool raw, char *buf, size_t len) override;
    virtual esp_err_t get_connected(bool *connected) override;
    virtual esp_err_t set_connected(bool connected) override;
    virtual esp_err_t get_description(char *buf, size_t len) override;
    virtual esp_err_t get_driverinfo(char *buf, size_t len) override;
    virtual esp_err_t get_driverversion(char *buf, size_t len) override;
    virtual esp_err_t get_interfaceversion(uint32_t *version) override;
    virtual esp_err_t get_name(char *buf, size_t len) override;
    virtual esp_err_t get_supportedactions(std::vector<std::string> &actions) override;

    // Switch specific interface
    virtual esp_err_t get_maxswitch(int32_t *maxswitch) override;
    virtual esp_err_t get_canwrite(int32_t id, bool *canwrite) override;
    virtual esp_err_t get_getswitch(int32_t id, bool *getswitch) override;
    virtual esp_err_t get_getswitchdescription(int32_t id, char *buf, size_t len) override;
    virtual esp_err_t get_getswitchname(int32_t id, char *buf, size_t len) override;
    virtual esp_err_t get_getswitchvalue(int32_t id, double *value) override;
    virtual esp_err_t get_minswitchvalue(int32_t id, double *value) override;
    virtual esp_err_t get_maxswitchvalue(int32_t id, double *value) override;
    virtual esp_err_t put_setswitch(int32_t id, bool value) override;
    virtual esp_err_t put_setswitchname(int32_t id, const char *name) override;
    virtual esp_err_t put_setswitchvalue(int32_t id, double value) override;
    virtual esp_err_t get_switchstep(int32_t id, double *switchstep) override;

private:
    bool _connected;
    int _num_switches;
    
    bool *_switch_states;
    char **_switch_names;
    char **_switch_descriptions;
    double *_switch_values;
    bool *_switch_can_write;
    double *_min_switch_values;
    double *_max_switch_values;
    double *_switch_steps;
    
    // GPIO pins for the switches
    int *_switch_pins;
};

#endif // ALPACA_SWITCH_H