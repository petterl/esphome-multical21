# Multical21 ESPHome Component

ESPHome component for reading Kamstrup Multical 21 water meters via CC1101 radio (wM-Bus Mode C1, 868.95 MHz).

## Sensors

| Sensor | Unit | Description |
|--------|------|-------------|
| `total_consumption` | m³ | Total water consumption |
| `month_start_value` | m³ | Consumption at month start |
| `water_temperature` | °C | Water temperature |
| `ambient_temperature` | °C | Ambient temperature |
| `current_flow` | L/h | Current flow rate |
| `last_update` | text | Last update timestamp |

## Quick Start

```yaml
external_components:
  - source: github://petterl/esp32-multical21@master
    components: [multical21]

spi:
  clk_pin: GPIO18
  mosi_pin: GPIO23
  miso_pin: GPIO19

multical21:
  cs_pin: GPIO4
  gdo0_pin: GPIO32
  meter_id: "12345678"  # From meter sticker
  key: "00112233..."    # From water provider

sensor:
  - platform: multical21
    total_consumption:
      name: "Water Total"
    current_flow:
      name: "Water Flow"
```

## Wiring

| CC1101 | ESP32 | ESP8266 (D1 Mini) |
|--------|-------|-------------------|
| VCC | 3.3V | 3.3V |
| GND | GND | GND |
| CSN | GPIO4 | D8 (GPIO15) |
| MOSI | GPIO23 | D7 (GPIO13) |
| MISO | GPIO19 | D6 (GPIO12) |
| SCK | GPIO18 | D5 (GPIO14) |
| GDO0 | GPIO32 | D2 (GPIO4) |

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
