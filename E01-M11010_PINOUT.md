# E01-M11010 Wireless Module Pinout

## Module Overview

The **E01-M11010** is a compact 868MHz wireless transceiver module based on the TI CC1101 chip. It's designed for IoT applications including smart metering, home automation, and wireless sensor networks.

**Specifications:**
- Frequency: 868MHz (other variants: 433MHz, 915MHz)
- Modulation: 2-FSK, GFSK, MSK, OOK, ASK
- Power Supply: 1.8V - 3.6V DC
- Interface: SPI
- Dimensions: 11mm × 10mm
- Range: Up to 1000m (open space)

## Pinout Diagram

```
E01-M11010 Module
   ___________
  |           |
1 | GND       |
2 | VCC       |
3 | GDO0      |
4 | CSN       |
5 | SCK       |
6 | MOSI      |
7 | MISO/GDO1 |
8 | GDO2      |
  |___________|
```

## Pin Descriptions

| Pin | Name      | Type   | Description                                          |
|-----|-----------|--------|------------------------------------------------------|
| 1   | GND       | Power  | Ground reference                                     |
| 2   | VCC       | Power  | Power supply: 1.8V - 3.6V DC (3.3V recommended)      |
| 3   | GDO0      | Output | Configurable digital output (sync word detection)    |
| 4   | CSN       | Input  | SPI Chip Select (active low)                         |
| 5   | SCK       | Input  | SPI Clock                                            |
| 6   | MOSI      | Input  | SPI Master Out Slave In (data to module)             |
| 7   | MISO/GDO1 | Output | SPI Master In Slave Out (data from module) / GDO1    |
| 8   | GDO2      | Output | Configurable digital output (not used in this project)|

## Important Notes

### Power Supply
- **Operating Voltage**: 1.8V - 3.6V DC
- **Recommended**: 3.3V (compatible with ESP32/ESP8266 logic levels)
- **Current Draw**:
  - Sleep mode: <1 µA
  - Idle mode: 1.5 mA
  - RX mode: 15 mA
  - TX mode: 30 mA (at max power)
- **Add decoupling capacitor**: 100nF ceramic + 10µF electrolytic near VCC pin

### SPI Interface
- **SPI Mode**: 0 (CPOL=0, CPHA=0)
- **Max Clock Speed**: 10 MHz
- **Data Rate**: 1 MHz recommended for reliability
- **CSN**: Active LOW (pull high when not in use)

### GDO Pins
- **GDO0**: Used in this project for packet detection (sync word received)
- **GDO1**: Shared with MISO - only available when not using SPI
- **GDO2**: Not used in this project

### Antenna
- **Required**: External antenna (50Ω impedance)
- **Connector**: Usually U.FL/IPEX
- **Length**: ~82mm for 868MHz (λ/4 monopole)

## Wiring for ESP32-C3 Super Mini

| E01-M11010 Pin | Pin Name  | ESP32-C3 GPIO | ESP32-C3 Pin |
|----------------|-----------|---------------|--------------|
| 1              | GND       | GND           | GND          |
| 2              | VCC       | 3.3V          | 3V3          |
| 3              | GDO0      | GPIO10        | 10           |
| 4              | CSN       | GPIO7         | 7            |
| 5              | SCK       | GPIO4         | 4            |
| 6              | MOSI      | GPIO6         | 6            |
| 7              | MISO/GDO1 | GPIO5         | 5            |
| 8              | GDO2      | Not Connected | -            |

### ESP32-C3 Configuration (multical21.yaml)

```yaml
spi:
  id: spi_bus
  clk_pin: GPIO4
  mosi_pin: GPIO6
  miso_pin: GPIO5

multical21:
  id: water_meter
  spi_id: spi_bus
  cs_pin: GPIO7
  gdo0_pin: GPIO10
  meter_id: ${meter_id}
  key: ${meter_key}
```

## Wiring for ESP8266 (D1 Mini / NodeMCU)

| E01-M11010 Pin | Pin Name  | ESP8266 GPIO | D1 Mini Pin | NodeMCU Pin |
|----------------|-----------|--------------|-------------|-------------|
| 1              | GND       | GND          | G           | GND         |
| 2              | VCC       | 3.3V         | 3V3         | 3V3         |
| 3              | GDO0      | GPIO4        | D2          | D2          |
| 4              | CSN       | GPIO15       | D8          | D8          |
| 5              | SCK       | GPIO14       | D5          | D5          |
| 6              | MOSI      | GPIO13       | D7          | D7          |
| 7              | MISO/GDO1 | GPIO12       | D6          | D6          |
| 8              | GDO2      | Not Connected| -           | -           |

### ESP8266 Configuration (multical21_esp8266.yaml)

```yaml
spi:
  id: spi_bus
  clk_pin: GPIO14   # D5
  mosi_pin: GPIO13  # D7
  miso_pin: GPIO12  # D6

multical21:
  id: water_meter
  spi_id: spi_bus
  cs_pin: GPIO15    # D8
  gdo0_pin: GPIO4   # D2
  meter_id: ${meter_id}
  key: ${meter_key}
```

## Comparison: E01-M11010 vs Standard CC1101

| Feature              | E01-M11010          | Standard CC1101 Module |
|----------------------|---------------------|------------------------|
| Chip                 | TI CC1101           | TI CC1101              |
| Size                 | 11mm × 10mm         | 16mm × 16mm (typical)  |
| Pinout               | 8-pin               | 8-10 pin (varies)      |
| Antenna              | External (U.FL)     | Spring antenna or SMA  |
| Power Supply         | 1.8V - 3.6V         | 1.8V - 3.6V            |
| SPI Interface        | Yes                 | Yes                    |
| Programming Required | No (uses CC1101)    | No (uses CC1101)       |
| Pin Compatibility    | ⚠️ Different order   | Standard layout        |

### Key Differences

1. **Pinout Order**: E01-M11010 has a different pin ordering than typical CC1101 breakout boards
2. **Compact Size**: Smaller form factor (11mm × 10mm)
3. **External Antenna**: Requires U.FL antenna connection
4. **GDO1/MISO Shared**: Pin 7 serves dual purpose (standard in CC1101)

## Troubleshooting

### Module Not Detected
- ✓ Check power supply is 3.3V (not 5V)
- ✓ Verify all SPI connections (especially CSN must be LOW to communicate)
- ✓ Add 100nF capacitor between VCC and GND
- ✓ Check that MISO pin is not damaged (bidirectional pin 7)
- ✓ Measure VCC pin should show 3.3V

### No Data Received
- ✓ Ensure antenna is properly connected
- ✓ Check antenna length (~82mm for 868MHz)
- ✓ Verify GDO0 connection for packet detection
- ✓ Check meter is within range (<50m for testing)
- ✓ Confirm frequency matches your region (868MHz EU, 915MHz US)

### Intermittent Connection
- ✓ Add larger decoupling capacitor (10µF + 100nF)
- ✓ Use shorter wires for SPI connections (<10cm)
- ✓ Keep module away from noise sources (switching power supplies)
- ✓ Ensure stable 3.3V power supply (minimum 50mA capacity)

## Module Configuration in CC1101

The E01-M11010 uses the standard CC1101 chip, so all CC1101 registers and configurations apply:

- **Base Frequency**: 868.95 MHz (for wM-Bus Mode C1)
- **Modulation**: 2-GFSK
- **Data Rate**: ~100 kbps
- **Sync Word**: 0x543D (wM-Bus preamble)
- **Packet Format**: Variable length
- **GDO0 Config**: Sync word received/transmitted

All register configurations are handled by the Multical21 component automatically.

## Safety Notes

⚠️ **Important Safety Information:**

1. **Never exceed 3.6V** on VCC pin - will damage module
2. **Never apply 5V** to any pin - use 3.3V logic only
3. **Static Sensitive** - handle with ESD precautions
4. **RF Exposure** - keep antenna away from body during transmission
5. **Regional Compliance** - ensure 868MHz operation is legal in your region

## Purchase Information

The E01-M11010 module is available from various suppliers:
- AliExpress, eBay, Amazon
- Typical cost: $2-5 per module
- Also sold as: E01-ML01DP5, E01-ML01D (variants with different frequencies)

## References

- TI CC1101 Datasheet: [www.ti.com](https://www.ti.com/product/CC1101)
- wM-Bus Standard: EN 13757-4
- ESPHome Documentation: [esphome.io](https://esphome.io)

## Module Photos Reference

```
Top View:
 ___________
|  E01-M11010 |
|             |
|  [CC1101]   |
|             |
|   [U.FL]    |
|_____________|

Pin Side View:
1 2 3 4 5 6 7 8
█ █ █ █ █ █ █ █
```

---

**Note**: Always verify your specific E01-M11010 variant pinout with manufacturer documentation, as some clones may have different pin arrangements.
