# Multical21 ESPHome Component

ESPHome component for reading Kamstrup Multical 21 water meters via CC1101 radio (wM-Bus Mode C1, 868.95 MHz).

## Sensors

| Sensor | Unit | Category | Description |
|--------|------|----------|-------------|
| `total_consumption` | m³ | Primary | Total water consumption |
| `month_start_value` | m³ | Primary | Consumption at month start |
| `water_temperature` | °C | Primary | Water temperature |
| `ambient_temperature` | °C | Primary | Ambient temperature |
| `current_flow` | L/h | Primary | Current flow rate |
| `last_update` | text | Diagnostic | Reading counter and uptime |
| `frames_received` | count | Diagnostic | Successfully received frames |
| `crc_errors` | count | Diagnostic | CRC validation failures |
| `signal_quality` | % | Diagnostic | Frame success rate |

All sensors are optional. Icons are set automatically.

## Quick Start

```yaml
external_components:
  - source: github://petterl/esphome-multical21@master
    components: [multical21]

spi:
  clk_pin: GPIO4
  mosi_pin: GPIO6
  miso_pin: GPIO5

multical21:
  cs_pin: GPIO7
  gdo0_pin: GPIO10
  meter_id: "12345678"  # 8 hex chars from meter sticker
  key: "00112233..."    # 32 hex chars from water provider

sensor:
  - platform: multical21
    total_consumption:
      name: "Water Total"
    current_flow:
      name: "Water Flow"
```

## Input Validation

The component validates `meter_id` (exactly 8 hex characters) and `key` (exactly 32 hex characters) at compile time. Wrong formats produce clear error messages.

## Wiring

| CC1101 | ESP32 | ESP32-C3 | ESP8266 (D1 Mini) |
|--------|-------|----------|-------------------|
| VCC | 3.3V | 3.3V | 3.3V |
| GND | GND | GND | GND |
| CSN | GPIO4 | GPIO7 | D8 (GPIO15) |
| MOSI | GPIO23 | GPIO6 | D7 (GPIO13) |
| MISO | GPIO19 | GPIO5 | D6 (GPIO12) |
| SCK | GPIO18 | GPIO4 | D5 (GPIO14) |
| GDO0 | GPIO32 | GPIO10 | D2 (GPIO4) |

## Daily Tracking

Use Home Assistant's [utility_meter](https://www.home-assistant.io/integrations/utility_meter/):

```yaml
utility_meter:
  daily_water:
    source: sensor.water_total
    cycle: daily
```

## Credits

Based on work by:
- [Patrik Thalin](https://github.com/pthalin/esp32-multical21) - Original ESP32 implementation
- [Chester](https://github.com/chester4444/esp-multical21) - wM-Bus protocol implementation
