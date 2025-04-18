# ESP32 ASCOM Alpaca Switch Controller

This project implements an ASCOM Alpaca compatible switch controller using an ESP32 WROOM microcontroller. It allows you to control multiple GPIO pins as switches via the standard ASCOM Alpaca protocol, making it compatible with astronomy software that supports ASCOM devices.

## Features

- Configurable number of switches (default: 5)
- Per-switch configuration for:
  - Normal state (normally on/off)
  - Minimum value
  - Maximum value
  - Step value
  - Writable state
- WiFi connectivity with DHCP or static IP support
- Authentication for secure access
- ASCOM Alpaca protocol compliance
- Automatic network discovery

## Hardware Requirements

- ESP32 WROOM development board
- Optional: Relays or other circuitry connected to GPIO pins for controlling external hardware

## Configuration

All configuration options are in `include/config.h`:

### Network Configuration
- `WIFI_SSID`: Your WiFi network name
- `WIFI_PASS`: Your WiFi password
- `USE_STATIC_IP`: Uncomment to enable static IP configuration
- `STATIC_IP`: Static IP address (if enabled)
- `STATIC_GATEWAY`: Network gateway (if using static IP)
- `STATIC_NETMASK`: Network mask (if using static IP)
- `STATIC_DNS1`: Primary DNS server (if using static IP)
- `STATIC_DNS2`: Secondary DNS server (if using static IP)

### HTTP Server Configuration
- `HTTP_SERVER_PORT`: HTTP server port (default: 80)

### Switch Configuration
- `DEFAULT_NUM_SWITCHES`: Number of switches (default: 5)
- `DEFAULT_SWITCH_PINS`: Default GPIO pin assignments
- `DEFAULT_SWITCH_NORMAL_STATES`: Default normal state for each switch
- `DEFAULT_SWITCH_MIN_VALUES`: Minimum value for each switch
- `DEFAULT_SWITCH_MAX_VALUES`: Maximum value for each switch
- `DEFAULT_SWITCH_STEPS`: Step value for each switch
- `DEFAULT_SWITCH_CAN_WRITE`: Whether each switch is writable

### Device Information
- `DEVICE_SERIAL`: Device serial number
- `DEVICE_NAME`: Device name
- `DEVICE_ORGANIZATION`: Organization name
- `DEVICE_LOCATION`: Device location description

## Pin Configuration

Default GPIO pin configuration:
- Switch 0: GPIO 12 (normally off, min=0.0, max=1.0, step=1.0, writable)
- Switch 1: GPIO 14 (normally off, min=0.0, max=1.0, step=1.0, writable)
- Switch 2: GPIO 27 (normally off, min=0.0, max=1.0, step=1.0, writable)
- Switch 3: GPIO 26 (normally off, min=0.0, max=1.0, step=1.0, writable)
- Switch 4: GPIO 25 (normally off, min=0.0, max=1.0, step=1.0, writable)

## Building and Flashing

This project uses PlatformIO for building and flashing. To build and flash the firmware:

1. Install [PlatformIO](https://platformio.org/install)
2. Clone this repository
3. Open the project in PlatformIO
4. Build and upload:

```bash
# Build the project
pio run

# Upload to ESP32
pio run --target upload

# Monitor serial output
pio run --target monitor
```

## Initial Setup

1. Update the network configuration in `include/config.h`:
   - Set your WiFi credentials
   - For static IP, uncomment `USE_STATIC_IP` and set related settings
2. Configure the number of switches and their settings in `include/config.h` if needed
3. Rebuild and flash the firmware to the ESP32
4. The ESP32 will connect to your WiFi network on startup

## Usage

Once connected to your WiFi network:

1. Connect to the device from ASCOM compatible astronomy software using:
   - Protocol: Alpaca
   - Device Type: Switch
   - IP Address: [ESP32-IP-ADDRESS] (or your configured static IP)
   - Port: 80

Note: Switches will return to their configured normal state (on/off) after a reboot.

## ASCOM Alpaca API

The controller implements the standard ASCOM Alpaca API for switch devices. The base URL for Alpaca API access is:

```
http://[ESP32-IP-ADDRESS]/api/v1/
```

## License

This project is licensed under the terms specified in the LICENSE file.

## Credits

- Uses the [Dark Dragons Astro Alpaca Server](https://github.com/darkdragonsastro/python-alpaca-server) library for ASCOM Alpaca protocol support
- ESP-IDF framework by Espressif Systems 