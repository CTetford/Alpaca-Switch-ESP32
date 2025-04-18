#pragma once

// Network Configuration
#define WIFI_SSID "your_wifi_ssid"
#define WIFI_PASS "your_wifi_password"

// Static IP Configuration (comment out to use DHCP)
// #define USE_STATIC_IP
#ifdef USE_STATIC_IP
    #define STATIC_IP "192.168.1.100"
    #define STATIC_GATEWAY "192.168.1.1"
    #define STATIC_NETMASK "255.255.255.0"
    #define STATIC_DNS1 "8.8.8.8"
    #define STATIC_DNS2 "8.8.4.4"
#endif

// HTTP Server Configuration
#define HTTP_SERVER_PORT 80

// Switch Configuration
#define DEFAULT_NUM_SWITCHES 5

// Default GPIO pins for switches
const int DEFAULT_SWITCH_PINS[DEFAULT_NUM_SWITCHES] = {12, 14, 27, 26, 25};

// Default normal states for each switch (true = normally on, false = normally off)
const bool DEFAULT_SWITCH_NORMAL_STATES[DEFAULT_NUM_SWITCHES] = {false, false, false, false, false};

// Default minimum values for each switch
const double DEFAULT_SWITCH_MIN_VALUES[DEFAULT_NUM_SWITCHES] = {0.0, 0.0, 0.0, 0.0, 0.0};

// Default maximum values for each switch
const double DEFAULT_SWITCH_MAX_VALUES[DEFAULT_NUM_SWITCHES] = {1.0, 1.0, 1.0, 1.0, 1.0};

// Default step values for each switch
const double DEFAULT_SWITCH_STEPS[DEFAULT_NUM_SWITCHES] = {1.0, 1.0, 1.0, 1.0, 1.0};

// Default writable state for each switch
const bool DEFAULT_SWITCH_CAN_WRITE[DEFAULT_NUM_SWITCHES] = {true, true, true, true, true};

// Device Information
#define DEVICE_SERIAL "ESP32_SWITCH_SERIAL"
#define DEVICE_NAME "ESP32 Alpaca Switch Server"
#define DEVICE_ORGANIZATION "YourName/Organization"
#define DEVICE_LOCATION "Location Description" 