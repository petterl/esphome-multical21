# ESPHome Kamstrup Multical 21 Water Meter Reader

<img align="right" height="250" src="images/kamstrup_multical21.png">

### Features

- Native ESPHome integration with Home Assistant
- Automatic device discovery
- OTA updates support
- Support for AES-128 decryption (with valid key)
- CRC check of received data
- Wireless reading of data via wM-Bus Mode C1
- Easy to build and configure

### Parts

Use these affiliate links to support chester4444 for his work!\
[CC1101 Module](https://s.click.aliexpress.com/e/_oDW0qJ2) \
[ESP32-C3 Super Mini](https://s.click.aliexpress.com/e/_c3HOPvoX) \
Some cables

### Wiring

#### ESP32-C3 Super Mini

| CC1101 | ESP32-C3 Super Mini |
| ------ | ------------------- |
| VCC    | 3V3                 |
| GND    | GND                 |
| CSN    | 7                   |
| MOSI   | 6                   |
| MISO   | 5                   |
| SCK    | 4                   |
| GD0    | 10                  |
| GD2    | Not Connected       |

<img height="300" src="images/esp32_c3_mini.jpg"> <img height="300" src="images/esp32_c3_mini_pinout.jpg">

### Installation

See the complete installation guide in the [esphome](esphome) directory:

- [ESPHome README](esphome/README.md) - Full installation and configuration guide

#### Quick Start

1. Copy `esphome/secrets.yaml.example` to `esphome/secrets.yaml` and fill in your WiFi credentials
2. Edit `esphome/multical21.yaml` with your meter ID and AES encryption key
3. Build and flash:
   ```bash
   cd esphome
   esphome run multical21.yaml
   ```

### Home Assistant

This ESPHome integration automatically discovers and registers all sensors with Home Assistant. No manual MQTT configuration needed!

The following entities are created automatically:

- **Total Water Consumption** - Current total in m³
- **Month Start Value** - Billing period start value in m³
- **Water Temperature** - Flow temperature in °C
- **Ambient Temperature** - Room temperature in °C
- Plus WiFi signal, uptime, and diagnostic sensors

Donation using [Ko-Fi](https://ko-fi.com/patriksretrotech) or [PayPal](https://www.paypal.com/donate/?business=UCTJFD6L7UYFL&no_recurring=0&item_name=Please+support+me%21&currency_code=SEK) are highly appreciated!

This is a based on [chester4444/esp-multical21](https://github.com/chester4444/esp-multical21).
Thanks to chester4444for his effort.
