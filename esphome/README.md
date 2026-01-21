# ESPHome Multical21 Water Meter Reader

This is an ESPHome implementation of the Kamstrup Multical 21 water meter reader using a CC1101 radio module to receive wM-Bus Mode C1 transmissions.

## Features

- Native ESPHome integration with Home Assistant
- Automatic discovery of sensors
- OTA updates
- Captive portal for WiFi configuration
- Status LED indication
- All original functionality preserved:
  - Total water consumption (m³)
  - Month start value (m³)
  - Water temperature (°C)
  - Ambient temperature (°C)

## Hardware Requirements

- ESP32-C3 Super Mini (or other ESP32 variant)
- CC1101 Sub-GHz transceiver module
- Connecting wires

## Wiring

### ESP32-C3 Super Mini

| CC1101 Pin | ESP32-C3 Pin |
|------------|--------------|
| VCC        | 3V3          |
| GND        | GND          |
| CSN        | GPIO 7       |
| MOSI       | GPIO 6       |
| MISO       | GPIO 5       |
| SCK        | GPIO 4       |
| GDO0       | GPIO 10      |
| GD2        | Not connected|

### Standard ESP32

| CC1101 Pin | ESP32 Pin    |
|------------|--------------|
| VCC        | 3V3          |
| GND        | GND          |
| CSN        | GPIO 4       |
| MOSI       | GPIO 23      |
| MISO       | GPIO 19      |
| SCK        | GPIO 18      |
| GDO0       | GPIO 32      |
| GD2        | Not connected|

## Installation

### 1. Prepare secrets.yaml

Copy `secrets.yaml.example` to `secrets.yaml` and fill in your credentials:

```yaml
wifi_ssid: "your_wifi_ssid"
wifi_password: "your_wifi_password"
api_encryption_key: "your_32_byte_base64_key=="
ota_password: "your_ota_password"
ap_password: "your_fallback_ap_password"
```

Generate an API encryption key:
```bash
openssl rand -base64 32
```

### 2. Configure Meter Credentials

Edit the `substitutions` section in `multical21.yaml`:

```yaml
substitutions:
  device_name: watermeter
  friendly_name: "Water Meter"
  # Your meter ID (4 bytes hex, e.g., "12345678")
  meter_id: "12345678"
  # Your AES-128 key (32 hex chars, obtain from water provider)
  meter_key: "00112233445566778899AABBCCDDEEFF"
```

**Important:** You must obtain your meter's encryption key from your water utility provider.

### 3. Build and Upload

Using ESPHome CLI:
```bash
cd esphome
esphome run multical21.yaml
```

Or using ESPHome Dashboard in Home Assistant:
1. Copy the entire `esphome` folder contents to your ESPHome configuration directory
2. Add `multical21.yaml` as a new device
3. Install the firmware

### 4. Using a Different ESP32 Variant

For standard ESP32 (not C3), modify the YAML:

```yaml
esp32:
  board: esp32dev
  framework:
    type: esp-idf
    version: recommended

# SPI bus for CC1101
spi:
  clk_pin: GPIO18
  mosi_pin: GPIO23
  miso_pin: GPIO19

# Multical21 component
multical21:
  id: water_meter
  cs_pin: GPIO4
  gdo0_pin: GPIO32
  meter_id: ${meter_id}
  key: ${meter_key}

# Status LED
status_led:
  pin: GPIO2
```

## Home Assistant Integration

Once the device is running and connected to Home Assistant, it will automatically create the following entities:

### Sensors
- **Total Water Consumption** - Current total consumption in m³ (state_class: total_increasing)
- **Month Start Value** - Reading at the start of billing period in m³
- **Water Temperature** - Flow temperature in °C
- **Ambient Temperature** - Ambient/room temperature in °C
- **WiFi Signal** - WiFi signal strength in dBm
- **Uptime** - Device uptime

### Text Sensors
- **Last Update** - Time since last meter reading
- **IP Address** - Device IP address
- **Connected SSID** - WiFi network name

### Binary Sensors
- **Status** - Online/offline status

### Buttons
- **Restart** - Restart the device

## Energy Dashboard Integration

To add water consumption to the Home Assistant Energy Dashboard:

1. Go to **Settings** → **Dashboards** → **Energy**
2. Click **Add Water Source**
3. Select **Total Water Consumption** sensor
4. The sensor is already configured with `state_class: total_increasing` and `device_class: water`

## Troubleshooting

### No readings received
- Verify your meter ID matches your physical meter
- Check that you have the correct AES encryption key from your provider
- Ensure wiring is correct, especially SPI connections
- Check debug logs for CC1101 initialization errors

### CC1101 not responding
- Verify 3.3V power supply (NOT 5V!)
- Check SPI wiring (MOSI, MISO, SCK, CS)
- Try adding a small capacitor (100nF) between VCC and GND on CC1101

### Weak signal
- Position the device closer to the water meter
- Some CC1101 modules have an antenna connector - use an appropriate 868 MHz antenna

### Debug Logging

The default log level is DEBUG. Check ESPHome logs for detailed information:
```bash
esphome logs multical21.yaml
```

## Technical Details

- **Protocol:** Wireless M-Bus (wM-Bus) Mode C1
- **Frequency:** 868.95 MHz
- **Modulation:** 2-GFSK
- **Data Rate:** ~103 kbps
- **Encryption:** AES-128 CTR mode
- **CRC:** EN13757 standard

## File Structure

```
esphome/
├── multical21.yaml           # Main ESPHome configuration
├── secrets.yaml.example      # Template for secrets
├── README.md                 # This file
└── components/
    └── multical21/
        ├── __init__.py       # Component definition
        ├── multical21.h      # C++ header
        ├── multical21.cpp    # C++ implementation (CC1101, AES, wM-Bus)
        ├── sensor.py         # Sensor platform
        └── text_sensor.py    # Text sensor platform
```

## License

This project maintains the same GPL-3.0 license as the original PlatformIO version.

## Credits

Based on the original [esp32-multical21](https://github.com/chester4444/esp32-multical21) project by chester4444.
