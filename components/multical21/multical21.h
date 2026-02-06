// Multical21 ESPHome Component
// Kamstrup Multical 21 water meter reader via CC1101 (wM-Bus Mode C1)
//
// Based on work by:
//   Patrik Thalin - https://github.com/pthalin/esp32-multical21
//   Chester - https://github.com/chester4444/esp-multical21

#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/spi/spi.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include <vector>
#include <mbedtls/aes.h>

namespace esphome {
namespace multical21 {

// Component version (update on each release)
static const char *const VERSION = "1.0.0";

// CC1101 Register addresses
static const uint8_t CC1101_IOCFG2 = 0x00;
static const uint8_t CC1101_IOCFG0 = 0x02;
static const uint8_t CC1101_FIFOTHR = 0x03;
static const uint8_t CC1101_SYNC1 = 0x04;
static const uint8_t CC1101_SYNC0 = 0x05;
static const uint8_t CC1101_PKTLEN = 0x06;
static const uint8_t CC1101_PKTCTRL1 = 0x07;
static const uint8_t CC1101_PKTCTRL0 = 0x08;
static const uint8_t CC1101_ADDR = 0x09;
static const uint8_t CC1101_CHANNR = 0x0A;
static const uint8_t CC1101_FSCTRL1 = 0x0B;
static const uint8_t CC1101_FSCTRL0 = 0x0C;
static const uint8_t CC1101_FREQ2 = 0x0D;
static const uint8_t CC1101_FREQ1 = 0x0E;
static const uint8_t CC1101_FREQ0 = 0x0F;
static const uint8_t CC1101_MDMCFG4 = 0x10;
static const uint8_t CC1101_MDMCFG3 = 0x11;
static const uint8_t CC1101_MDMCFG2 = 0x12;
static const uint8_t CC1101_MDMCFG1 = 0x13;
static const uint8_t CC1101_MDMCFG0 = 0x14;
static const uint8_t CC1101_DEVIATN = 0x15;
static const uint8_t CC1101_MCSM1 = 0x17;
static const uint8_t CC1101_MCSM0 = 0x18;
static const uint8_t CC1101_FOCCFG = 0x19;
static const uint8_t CC1101_BSCFG = 0x1A;
static const uint8_t CC1101_AGCCTRL2 = 0x1B;
static const uint8_t CC1101_AGCCTRL1 = 0x1C;
static const uint8_t CC1101_AGCCTRL0 = 0x1D;
static const uint8_t CC1101_FREND1 = 0x21;
static const uint8_t CC1101_FREND0 = 0x22;
static const uint8_t CC1101_FSCAL3 = 0x23;
static const uint8_t CC1101_FSCAL2 = 0x24;
static const uint8_t CC1101_FSCAL1 = 0x25;
static const uint8_t CC1101_FSCAL0 = 0x26;
static const uint8_t CC1101_FSTEST = 0x29;
static const uint8_t CC1101_TEST2 = 0x2C;
static const uint8_t CC1101_TEST1 = 0x2D;
static const uint8_t CC1101_TEST0 = 0x2E;

// CC1101 Status registers
static const uint8_t CC1101_MARCSTATE = 0x35;
static const uint8_t CC1101_RXBYTES = 0x3B;

// CC1101 Strobe commands
static const uint8_t CC1101_SRES = 0x30;
static const uint8_t CC1101_SCAL = 0x33;
static const uint8_t CC1101_SRX = 0x34;
static const uint8_t CC1101_SIDLE = 0x36;
static const uint8_t CC1101_SFRX = 0x3A;

// CC1101 FIFO
static const uint8_t CC1101_RXFIFO = 0x3F;

// Register access modes
static const uint8_t READ_SINGLE = 0x80;
static const uint8_t READ_BURST = 0xC0;

// MARCSTATE values
static const uint8_t MARCSTATE_IDLE = 0x01;
static const uint8_t MARCSTATE_RX = 0x0D;

// wM-Bus Mode C1 preamble
static const uint8_t WMBUS_PREAMBLE_1 = 0x54;
static const uint8_t WMBUS_PREAMBLE_2 = 0x3D;

// Maximum frame length
static const uint8_t MAX_FRAME_LENGTH = 64;

// CRC polynomial for EN13757
static const uint16_t CRC16_EN13757_POLY = 0x3D65;

// Compact frame (CI=0x79) field positions within decrypted payload
static const uint8_t COMPACT_FRAME_TYPE = 0x79;
static const uint8_t COMPACT_POS_TOTAL = 9;
static const uint8_t COMPACT_POS_TARGET = 13;
static const uint8_t COMPACT_POS_FLOW_TEMP = 17;
static const uint8_t COMPACT_POS_AMBIENT_TEMP = 18;
static const uint8_t COMPACT_MIN_LENGTH = 19;

// Long frame (CI=0x78) field positions within decrypted payload
static const uint8_t LONG_FRAME_TYPE = 0x78;
static const uint8_t LONG_POS_TOTAL = 10;
static const uint8_t LONG_POS_TARGET = 16;
static const uint8_t LONG_POS_FLOW_TEMP = 23;
static const uint8_t LONG_POS_AMBIENT_TEMP = 29;
static const uint8_t LONG_MIN_LENGTH = 30;

class Multical21Component : public PollingComponent,
                            public spi::SPIDevice<spi::BIT_ORDER_MSB_FIRST, spi::CLOCK_POLARITY_LOW,
                                                   spi::CLOCK_PHASE_LEADING, spi::DATA_RATE_1MHZ> {
 public:
  void setup() override;
  void loop() override;
  void update() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  void set_gdo0_pin(GPIOPin *pin) { this->gdo0_pin_ = pin; }
  void set_meter_id(const std::string &meter_id);
  void set_key(const std::string &key);

  void set_total_consumption_sensor(sensor::Sensor *sensor) { this->total_consumption_sensor_ = sensor; }
  void set_month_start_sensor(sensor::Sensor *sensor) { this->month_start_sensor_ = sensor; }
  void set_water_temp_sensor(sensor::Sensor *sensor) { this->water_temp_sensor_ = sensor; }
  void set_ambient_temp_sensor(sensor::Sensor *sensor) { this->ambient_temp_sensor_ = sensor; }
  void set_current_flow_sensor(sensor::Sensor *sensor) { this->current_flow_sensor_ = sensor; }
  void set_last_update_sensor(text_sensor::TextSensor *sensor) { this->last_update_sensor_ = sensor; }
  void set_frames_received_sensor(sensor::Sensor *sensor) { this->frames_received_sensor_ = sensor; }
  void set_crc_errors_sensor(sensor::Sensor *sensor) { this->crc_errors_sensor_ = sensor; }
  void set_signal_quality_sensor(sensor::Sensor *sensor) { this->signal_quality_sensor_ = sensor; }

 protected:
  // CC1101 communication
  void write_register(uint8_t reg, uint8_t value);
  uint8_t read_register(uint8_t reg);
  uint8_t read_status_register(uint8_t reg);
  void read_burst(uint8_t reg, uint8_t *buffer, uint8_t len);
  void send_strobe(uint8_t strobe);
  bool reset_cc1101();
  void init_cc1101_registers();
  void start_receiver();

  // Frame processing
  bool receive_frame();
  bool check_meter_id(const uint8_t *payload);
  bool decrypt_frame(const uint8_t *payload, uint8_t length);
  void parse_meter_data(const uint8_t *data, uint8_t length);

  // AES-128 CTR decryption (using mbedtls)
  void aes_ctr_decrypt(const uint8_t *cipher, uint8_t *plain, uint8_t length, const uint8_t *iv);

  // CRC calculation
  uint16_t crc16_en13757(const uint8_t *data, size_t length);

  // Utility
  void hex_to_bytes(const std::string &hex, uint8_t *bytes, size_t len);

  GPIOPin *gdo0_pin_{nullptr};

  uint8_t meter_id_[4]{0};
  uint8_t aes_key_[16]{0};
  mbedtls_aes_context aes_ctx_;  // mbedtls AES context

  // Sensors
  sensor::Sensor *total_consumption_sensor_{nullptr};
  sensor::Sensor *month_start_sensor_{nullptr};
  sensor::Sensor *water_temp_sensor_{nullptr};
  sensor::Sensor *ambient_temp_sensor_{nullptr};
  sensor::Sensor *current_flow_sensor_{nullptr};
  text_sensor::TextSensor *last_update_sensor_{nullptr};
  sensor::Sensor *frames_received_sensor_{nullptr};
  sensor::Sensor *crc_errors_sensor_{nullptr};
  sensor::Sensor *signal_quality_sensor_{nullptr};

  // State
  volatile bool packet_available_{false};
  bool cc1101_initialized_{false};
  uint8_t frame_buffer_[MAX_FRAME_LENGTH]{0};
  uint8_t plaintext_[MAX_FRAME_LENGTH]{0};

  // Last values
  float last_total_{0};
  float last_month_start_{0};
  uint8_t last_water_temp_{0};
  uint8_t last_ambient_temp_{0};

  // Flow calculation state
  float prev_total_{0};            // Previous total for flow calculation
  uint32_t prev_reading_time_{0};  // Time of previous reading (millis)

  // Diagnostics
  uint32_t frames_received_{0};
  uint32_t crc_errors_{0};
  uint32_t decrypt_errors_{0};
  uint32_t parse_errors_{0};
  uint32_t reading_count_{0};
};

}  // namespace multical21
}  // namespace esphome
