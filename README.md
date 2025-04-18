# ESP32 ASCOM Alpaca Switch Controller

This project implements an ASCOM Alpaca compatible switch controller using an ESP32 WROOM microcontroller. It allows you to control up to 5 GPIO pins as switches via the standard ASCOM Alpaca protocol, making it compatible with astronomy software that supports ASCOM devices.

## Features

- Controls up to 5 GPIO pins as switches
- WiFi connectivity
- Authentication for secure access
- Switch state persistence across power cycles
- ASCOM Alpaca protocol compliance
- Automatic network discovery

## Hardware Requirements

- ESP32 WROOM development board
- Optional: Relays or other circuitry connected to GPIO pins for controlling external hardware

## Pin Configuration

Default GPIO pin configuration:
- Switch 0: GPIO 12
- Switch 1: GPIO 14
- Switch 2: GPIO 27
- Switch 3: GPIO 26
- Switch 4: GPIO 25

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

1. Update the WiFi credentials in main.cpp (WIFI_SSID and WIFI_PASS constants)
2. Rebuild and flash the firmware to the ESP32
3. The ESP32 will connect to your WiFi network on startup

## Usage

Once connected to your WiFi network:

1. Connect to the device from ASCOM compatible astronomy software using:
   - Protocol: Alpaca
   - Device Type: Switch
   - IP Address: [ESP32-IP-ADDRESS]
   - Port: 80

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