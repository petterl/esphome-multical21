#include "multical21.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include <cstring>

namespace esphome {
namespace multical21 {

static const char *const TAG = "multical21";

void Multical21Component::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Multical21...");

  // Note: mbedtls AES context is initialized in set_key() which is called before setup()

  // Setup GDO0 pin
  if (this->gdo0_pin_ != nullptr) {
    this->gdo0_pin_->setup();
  }

  // Initialize SPI
  this->spi_setup();

  // Reset and initialize CC1101
  if (!this->reset_cc1101()) {
    ESP_LOGE(TAG, "Failed to reset CC1101!");
    this->mark_failed();
    return;
  }

  ESP_LOGI(TAG, "CC1101 reset successful");

  // Initialize CC1101 registers for wM-Bus Mode C1
  this->init_cc1101_registers();

  // Calibrate
  this->send_strobe(CC1101_SCAL);
  delay(1);

  // Start receiver
  this->start_receiver();

  this->cc1101_initialized_ = true;
  ESP_LOGI(TAG, "Multical21 setup complete");
}

void Multical21Component::loop() {
  if (!this->cc1101_initialized_) {
    return;
  }

  // Check GDO0 for packet available (simple polling - GDO0 LOW means sync detected)
  if (this->gdo0_pin_ != nullptr && !this->gdo0_pin_->digital_read()) {
    // Try to receive frame
    if (this->receive_frame()) {
      ESP_LOGD(TAG, "Valid frame received from meter");
    }
  }
}

void Multical21Component::update() {
  // Publish last known values if available
  if (this->last_total_ > 0 || this->last_month_start_ > 0) {
    // Values are published when received, this is just for periodic refresh
  }
}

void Multical21Component::dump_config() {
  ESP_LOGCONFIG(TAG, "Multical21:");
  LOG_PIN("  GDO0 Pin: ", this->gdo0_pin_);
  ESP_LOGCONFIG(TAG, "  Meter ID: %02X%02X%02X%02X", this->meter_id_[0], this->meter_id_[1], this->meter_id_[2],
                this->meter_id_[3]);
  if (this->cc1101_initialized_) {
    ESP_LOGCONFIG(TAG, "  CC1101: Initialized");
  } else {
    ESP_LOGCONFIG(TAG, "  CC1101: NOT initialized");
  }
}

void Multical21Component::set_meter_id(const std::string &meter_id) {
  if (meter_id.length() >= 8) {
    this->hex_to_bytes(meter_id, this->meter_id_, 4);
    ESP_LOGD(TAG, "Meter ID set: %02X%02X%02X%02X", this->meter_id_[0], this->meter_id_[1], this->meter_id_[2],
             this->meter_id_[3]);
  }
}

void Multical21Component::set_key(const std::string &key) {
  if (key.length() >= 32) {
    this->hex_to_bytes(key, this->aes_key_, 16);

    // Initialize mbedtls AES context and set encryption key
    mbedtls_aes_init(&this->aes_ctx_);
    mbedtls_aes_setkey_enc(&this->aes_ctx_, this->aes_key_, 128);

    // Log key for debugging
    ESP_LOGI(TAG, "AES key set: %02X%02X%02X%02X...%02X%02X%02X%02X",
             this->aes_key_[0], this->aes_key_[1], this->aes_key_[2], this->aes_key_[3],
             this->aes_key_[12], this->aes_key_[13], this->aes_key_[14], this->aes_key_[15]);
  }
}

void Multical21Component::hex_to_bytes(const std::string &hex, uint8_t *bytes, size_t len) {
  for (size_t i = 0; i < len && i * 2 + 1 < hex.length(); i++) {
    char byte_str[3] = {hex[i * 2], hex[i * 2 + 1], '\0'};
    bytes[i] = strtol(byte_str, nullptr, 16);
  }
}

void Multical21Component::write_register(uint8_t reg, uint8_t value) {
  this->enable();
  delayMicroseconds(5);
  this->transfer_byte(reg);
  this->transfer_byte(value);
  this->disable();
}

uint8_t Multical21Component::read_register(uint8_t reg) {
  this->enable();
  delayMicroseconds(5);
  this->transfer_byte(reg | READ_SINGLE);
  uint8_t value = this->transfer_byte(0x00);
  this->disable();
  return value;
}

uint8_t Multical21Component::read_status_register(uint8_t reg) {
  this->enable();
  delayMicroseconds(5);
  this->transfer_byte(reg | READ_BURST);
  uint8_t value = this->transfer_byte(0x00);
  this->disable();
  return value;
}

void Multical21Component::read_burst(uint8_t reg, uint8_t *buffer, uint8_t len) {
  this->enable();
  delayMicroseconds(5);
  this->transfer_byte(reg | READ_BURST);
  for (uint8_t i = 0; i < len; i++) {
    buffer[i] = this->transfer_byte(0x00);
  }
  delayMicroseconds(2);
  this->disable();
}

void Multical21Component::send_strobe(uint8_t strobe) {
  this->enable();
  delayMicroseconds(5);
  this->transfer_byte(strobe);
  delayMicroseconds(5);
  this->disable();
}

bool Multical21Component::reset_cc1101() {
  this->disable();
  delayMicroseconds(5);

  this->enable();
  delayMicroseconds(10);
  this->disable();
  delayMicroseconds(45);

  this->enable();
  delayMicroseconds(5);

  this->transfer_byte(CC1101_SRES);

  // Wait for reset to complete
  delay(10);

  this->disable();

  // Verify by reading a register
  uint8_t version = this->read_status_register(0x31);  // CC1101_VERSION
  ESP_LOGD(TAG, "CC1101 Version: 0x%02X", version);

  return (version == 0x14 || version == 0x04 || version == 0x03);
}

void Multical21Component::init_cc1101_registers() {
  // Configure CC1101 for wM-Bus Mode C1 (868.95 MHz, ~100 kbps)
  this->write_register(CC1101_IOCFG2, 0x2E);   // GDO2 - High impedance
  this->write_register(CC1101_IOCFG0, 0x06);   // GDO0 - Asserts when sync word has been sent/received
  this->write_register(CC1101_FIFOTHR, 0x00);  // RX FIFO threshold
  this->write_register(CC1101_PKTLEN, 0x30);   // Packet length (48 bytes)
  this->write_register(CC1101_PKTCTRL1, 0x00);
  this->write_register(CC1101_PKTCTRL0, 0x02);  // Infinite packet length mode
  this->write_register(CC1101_SYNC1, 0x54);    // Sync word high byte
  this->write_register(CC1101_SYNC0, 0x3D);    // Sync word low byte
  this->write_register(CC1101_ADDR, 0x00);
  this->write_register(CC1101_CHANNR, 0x00);

  // Frequency settings for 868.95 MHz
  this->write_register(CC1101_FSCTRL1, 0x08);
  this->write_register(CC1101_FSCTRL0, 0x00);
  this->write_register(CC1101_FREQ2, 0x21);
  this->write_register(CC1101_FREQ1, 0x6B);
  this->write_register(CC1101_FREQ0, 0xD0);

  // Modem configuration (~103 kbps, 2-GFSK)
  this->write_register(CC1101_MDMCFG4, 0x5C);
  this->write_register(CC1101_MDMCFG3, 0x04);
  this->write_register(CC1101_MDMCFG2, 0x06);
  this->write_register(CC1101_MDMCFG1, 0x22);
  this->write_register(CC1101_MDMCFG0, 0xF8);
  this->write_register(CC1101_DEVIATN, 0x44);

  // Main radio control state machine
  this->write_register(CC1101_MCSM1, 0x00);
  this->write_register(CC1101_MCSM0, 0x18);

  // Frequency offset compensation
  this->write_register(CC1101_FOCCFG, 0x2E);
  this->write_register(CC1101_BSCFG, 0xBF);

  // AGC control
  this->write_register(CC1101_AGCCTRL2, 0x43);
  this->write_register(CC1101_AGCCTRL1, 0x09);
  this->write_register(CC1101_AGCCTRL0, 0xB5);

  // Front end configuration
  this->write_register(CC1101_FREND1, 0xB6);
  this->write_register(CC1101_FREND0, 0x10);

  // Frequency synthesizer calibration
  this->write_register(CC1101_FSCAL3, 0xEA);
  this->write_register(CC1101_FSCAL2, 0x2A);
  this->write_register(CC1101_FSCAL1, 0x00);
  this->write_register(CC1101_FSCAL0, 0x1F);

  // Test registers
  this->write_register(CC1101_FSTEST, 0x59);
  this->write_register(CC1101_TEST2, 0x81);
  this->write_register(CC1101_TEST1, 0x35);
  this->write_register(CC1101_TEST0, 0x09);

  ESP_LOGD(TAG, "CC1101 registers initialized");
}

void Multical21Component::start_receiver() {
  // Enter IDLE state
  this->send_strobe(CC1101_SIDLE);

  // Wait for IDLE
  for (int i = 0; i < 100; i++) {
    if ((this->read_status_register(CC1101_MARCSTATE) & 0x1F) == MARCSTATE_IDLE) {
      break;
    }
    delay(1);
  }

  // Flush RX FIFO
  this->send_strobe(CC1101_SFRX);

  // Enter RX state
  this->send_strobe(CC1101_SRX);

  // Wait for RX
  for (int i = 0; i < 100; i++) {
    if ((this->read_status_register(CC1101_MARCSTATE) & 0x1F) == MARCSTATE_RX) {
      break;
    }
    delay(1);
  }
}

bool Multical21Component::receive_frame() {
  // Read preamble (should be 0x54 0x3D)
  uint8_t p1 = this->read_register(CC1101_RXFIFO);
  uint8_t p2 = this->read_register(CC1101_RXFIFO);

  if (p1 != WMBUS_PREAMBLE_1 || p2 != WMBUS_PREAMBLE_2) {
    this->start_receiver();
    return false;
  }

  // Read payload length
  uint8_t length = this->read_register(CC1101_RXFIFO);

  if (length >= MAX_FRAME_LENGTH || length < 18) {
    ESP_LOGW(TAG, "Invalid frame length: %d", length);
    this->start_receiver();
    return false;
  }

  // Read payload
  for (uint8_t i = 0; i < length; i++) {
    this->frame_buffer_[i] = this->read_register(CC1101_RXFIFO);
  }

  // Restart receiver
  this->start_receiver();

  // Log raw frame with hex dump (like old DEBUG_PRINTF)
  char hex_buf[MAX_FRAME_LENGTH * 3 + 1];
  for (uint8_t i = 0; i < length && i < MAX_FRAME_LENGTH; i++) {
    snprintf(&hex_buf[i * 2], 3, "%02x", this->frame_buffer_[i]);
  }
  hex_buf[length * 2] = '\0';
  ESP_LOGD(TAG, "Payload (%d bytes): %s", length, hex_buf);

  // Check meter ID
  if (!this->check_meter_id(this->frame_buffer_)) {
    ESP_LOGD(TAG, "Meter ID mismatch");
    return false;
  }

  ESP_LOGI(TAG, "Frame matched our meter ID");

  // Decrypt and process
  if (this->decrypt_frame(this->frame_buffer_, length)) {
    return true;
  }

  return false;
}

bool Multical21Component::check_meter_id(const uint8_t *payload) {
  // Meter ID is at bytes 3-6 in reverse order (little endian)
  // payload[6] = meter_id[0], payload[5] = meter_id[1], etc.
  for (uint8_t i = 0; i < 4; i++) {
    if (this->meter_id_[i] != payload[6 - i]) {
      return false;
    }
  }
  return true;
}

bool Multical21Component::decrypt_frame(const uint8_t *payload, uint8_t length) {
  if (length < 18) {
    return false;
  }

  // Cipher starts at index 16, subtract 2 for CRC bytes at end of frame
  // This matches the original: cipherLength = length - 2 - 16
  uint8_t cipher_length = length - 2 - 16;
  if (cipher_length > MAX_FRAME_LENGTH - 16 || cipher_length == 0) {
    return false;
  }

  // Build IV from payload (wM-Bus specific IV construction)
  uint8_t iv[16] = {0};
  memcpy(iv, &payload[1], 8);      // bytes 1-8
  iv[8] = payload[10];             // byte 10
  memcpy(&iv[9], &payload[12], 4); // bytes 12-15 (4 bytes)
  // Rest is padded with zeros

  // Log IV for debugging
  ESP_LOGD(TAG, "IV: %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
           iv[0], iv[1], iv[2], iv[3], iv[4], iv[5], iv[6], iv[7],
           iv[8], iv[9], iv[10], iv[11], iv[12], iv[13], iv[14], iv[15]);
  ESP_LOGD(TAG, "Cipher length: %d", cipher_length);

  // Decrypt using AES-128 CTR mode
  this->aes_ctr_decrypt(&payload[16], this->plaintext_, cipher_length, iv);

  // Log decrypted data (like old DEBUG_PRINTF)
  char hex_buf[MAX_FRAME_LENGTH * 2 + 1];
  for (uint8_t i = 0; i < cipher_length && i < MAX_FRAME_LENGTH; i++) {
    snprintf(&hex_buf[i * 2], 3, "%02x", this->plaintext_[i]);
  }
  hex_buf[cipher_length * 2] = '\0';
  ESP_LOGD(TAG, "Decrypted (%d bytes): %s", cipher_length, hex_buf);

  // Parse decrypted data
  this->parse_meter_data(this->plaintext_, cipher_length);

  return true;
}

void Multical21Component::aes_ctr_decrypt(const uint8_t *cipher, uint8_t *plain, uint8_t length, const uint8_t *iv) {
  uint8_t counter[16];
  uint8_t keystream[16];

  memcpy(counter, iv, 16);

  for (uint8_t i = 0; i < length; i += 16) {
    // Encrypt counter to get keystream block using mbedtls
    mbedtls_aes_crypt_ecb(&this->aes_ctx_, MBEDTLS_AES_ENCRYPT, counter, keystream);

    // XOR ciphertext with keystream
    uint8_t block_len = (length - i < 16) ? (length - i) : 16;
    for (uint8_t j = 0; j < block_len; j++) {
      plain[i + j] = cipher[i + j] ^ keystream[j];
    }

    // Increment counter
    for (int j = 15; j >= 0; j--) {
      if (++counter[j] != 0) {
        break;
      }
    }
  }
}

uint16_t Multical21Component::crc16_en13757(const uint8_t *data, size_t length) {
  uint16_t crc = 0x0000;

  for (size_t i = 0; i < length; i++) {
    uint8_t byte = data[i];
    for (int j = 0; j < 8; j++) {
      if (((crc >> 8) ^ (byte & 0x80 ? 0x80 : 0)) & 0x80) {
        crc = (crc << 1) ^ CRC16_EN13757_POLY;
      } else {
        crc = crc << 1;
      }
      byte <<= 1;
    }
  }

  return ~crc;
}

void Multical21Component::parse_meter_data(const uint8_t *data, uint8_t length) {
  // Determine frame type
  int pos_total = 9;
  int pos_target = 13;
  int pos_flow_temp = 17;
  int pos_ambient_temp = 18;

  if (length > 2) {
    if (data[2] == 0x79) {
      // Compact frame
      pos_total = 9;
      pos_target = 13;
      pos_flow_temp = 17;
      pos_ambient_temp = 18;
    } else if (data[2] == 0x78) {
      // Long frame
      pos_total = 10;
      pos_target = 16;
      pos_flow_temp = 23;
      pos_ambient_temp = 29;
    } else {
      ESP_LOGW(TAG, "Unknown frame type: 0x%02X", data[2]);
      return;
    }
  }

  // Verify CRC
  uint16_t calc_crc = this->crc16_en13757(data + 2, length - 2);
  uint16_t read_crc = (data[1] << 8) | data[0];

  if (calc_crc != read_crc) {
    ESP_LOGW(TAG, "CRC mismatch: calculated 0x%04X, read 0x%04X", calc_crc, read_crc);
    return;
  }

  ESP_LOGD(TAG, "CRC OK");

  // Parse values (little endian)
  uint32_t total_raw = data[pos_total] | (data[pos_total + 1] << 8) | (data[pos_total + 2] << 16) |
                       (data[pos_total + 3] << 24);
  uint32_t target_raw = data[pos_target] | (data[pos_target + 1] << 8) | (data[pos_target + 2] << 16) |
                        (data[pos_target + 3] << 24);

  float total_m3 = total_raw / 1000.0f;
  float target_m3 = target_raw / 1000.0f;
  uint8_t flow_temp = data[pos_flow_temp];
  uint8_t ambient_temp = data[pos_ambient_temp];

  ESP_LOGI(TAG, "Total: %.3f m³, Month start: %.3f m³, Water temp: %d°C, Ambient temp: %d°C", total_m3, target_m3,
           flow_temp, ambient_temp);

  // Store values
  this->last_total_ = total_m3;
  this->last_month_start_ = target_m3;
  this->last_water_temp_ = flow_temp;
  this->last_ambient_temp_ = ambient_temp;

  // Publish to sensors
  if (this->total_consumption_sensor_ != nullptr) {
    this->total_consumption_sensor_->publish_state(total_m3);
  }

  if (this->month_start_sensor_ != nullptr) {
    this->month_start_sensor_->publish_state(target_m3);
  }

  if (this->water_temp_sensor_ != nullptr) {
    this->water_temp_sensor_->publish_state(flow_temp);
  }

  if (this->ambient_temp_sensor_ != nullptr) {
    this->ambient_temp_sensor_->publish_state(ambient_temp);
  }

  // Publish last update time
  if (this->last_update_sensor_ != nullptr) {
    // Simple timestamp using millis
    uint32_t uptime_sec = millis() / 1000;
    uint32_t hours = uptime_sec / 3600;
    uint32_t minutes = (uptime_sec % 3600) / 60;
    uint32_t seconds = uptime_sec % 60;
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "Uptime: %02lu:%02lu:%02lu", (unsigned long)hours, (unsigned long)minutes, (unsigned long)seconds);
    this->last_update_sensor_->publish_state(buffer);
  }
}

}  // namespace multical21
}  // namespace esphome
