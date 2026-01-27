# ESPHome Kamstrup Multical 21 Water Meter Reader

<img align="right" height="250" src="images/kamstrup_multical21.png">

An ESPHome external component for wirelessly reading Kamstrup Multical 21 water meters using a CC1101 radio module.

## Features

- Native ESPHome integration with Home Assistant
- Automatic device discovery
- OTA updates support
- AES-128 decryption (requires key from water utility)
- CRC validation of received data
- Wireless reading via wM-Bus Mode C1 (868.95 MHz)

## Installation

### 1. Add to your ESPHome configuration

Add the external component to your ESPHome YAML:

```yaml
external_components:
  - source: github://petterl/esp32-multical21@master
    components: [multical21]
```

### 2. Configure the component

```yaml
# SPI bus for CC1101
spi:
  clk_pin: GPIO4
  mosi_pin: GPIO6
  miso_pin: GPIO5

# Multical21 water meter
multical21:
  id: water_meter
  cs_pin: GPIO7
  gdo0_pin: GPIO10
  meter_id: "12345678"        # Your meter ID (8 hex chars)
  key: "00112233445566778899AABBCCDDEEFF"  # AES key from water utility

sensor:
  - platform: multical21
    multical21_id: water_meter
    total_consumption:
      name: "Total Water Consumption"
    month_start_value:
      name: "Month Start Value"
    water_temperature:
      name: "Water Temperature"
    ambient_temperature:
      name: "Ambient Temperature"
    current_flow:
      name: "Current Water Flow"

text_sensor:
  - platform: multical21
    multical21_id: water_meter
    last_update:
      name: "Last Update"
```

### 3. Get your credentials

- **Meter ID**: Found on the sticker on your water meter (8 hex characters)
- **AES Key**: Contact your water utility provider to obtain the encryption key

## Example Configurations

Complete example configurations for different boards are available in the [examples/](examples/) folder:

| Board | Example File |
|-------|--------------|
| ESP32-C3 Super Mini | [esp32-c3.yaml](examples/esp32-c3.yaml) |
| Standard ESP32 | [esp32.yaml](examples/esp32.yaml) |
| ESP8266 (D1 Mini) | [esp8266.yaml](examples/esp8266.yaml) |

Copy the example that matches your board, create a `secrets.yaml` based on [secrets.yaml.example](examples/secrets.yaml.example), and flash it.

## Hardware

### Parts

- [CC1101 868MHz Module](https://s.click.aliexpress.com/e/_oDW0qJ2)
- [ESP32-C3 Super Mini](https://s.click.aliexpress.com/e/_c3HOPvoX) (or other ESP32/ESP8266)
- Connecting wires

### Wiring (ESP32-C3 Super Mini)

| CC1101 | ESP32-C3 |
|--------|----------|
| VCC    | 3V3      |
| GND    | GND      |
| CSN    | GPIO 7   |
| MOSI   | GPIO 6   |
| MISO   | GPIO 5   |
| SCK    | GPIO 4   |
| GDO0   | GPIO 10  |
| GD2    | NC       |

<img height="300" src="images/esp32_c3_mini.jpg"> <img height="300" src="images/esp32_c3_mini_pinout.jpg">

See [E01-M11010_PINOUT.md](E01-M11010_PINOUT.md) for detailed CC1101 module pinout and wiring for other boards.

## Home Assistant

The component automatically creates entities in Home Assistant:

- **Total Water Consumption** - Current total in m³ (Energy Dashboard compatible)
- **Month Start Value** - Billing period start value in m³
- **Water Temperature** - Flow temperature in °C
- **Ambient Temperature** - Room temperature in °C
- **Current Flow** - Flow rate in L/h

### Energy Dashboard

To add to the Energy Dashboard:
1. Go to **Settings** → **Dashboards** → **Energy**
2. Click **Add Water Source**
3. Select **Total Water Consumption** sensor

### Utility Meter

For daily/weekly/monthly tracking, add to your Home Assistant `configuration.yaml`:

```yaml
utility_meter:
  daily_water:
    source: sensor.water_meter_total_water_consumption
    cycle: daily
  monthly_water:
    source: sensor.water_meter_total_water_consumption
    cycle: monthly
```

## Troubleshooting

### No readings received
- Verify your meter ID matches your physical meter
- Check that you have the correct AES encryption key
- Ensure wiring is correct, especially SPI connections
- Check debug logs for CC1101 initialization errors

### CC1101 not responding
- Verify 3.3V power supply (NOT 5V!)
- Check SPI wiring (MOSI, MISO, SCK, CS)
- Try adding a 100nF capacitor between VCC and GND

### Weak signal
- Position the device closer to the water meter
- Use a proper 868 MHz antenna if your module has a connector

## Technical Details

| Parameter | Value |
|-----------|-------|
| Protocol | wM-Bus Mode C1 |
| Frequency | 868.95 MHz |
| Modulation | 2-GFSK |
| Data Rate | ~103 kbps |
| Encryption | AES-128 CTR |
| CRC | EN13757 |

## Credits

Based on work by:
- [Patrik Thalin](https://github.com/pthalin/esp32-multical21) - Original ESP32 implementation
- [Chester](https://github.com/chester4444/esp-multical21) - wM-Bus protocol implementation

## License

GPL-3.0 - See [LICENSE](LICENSE)
