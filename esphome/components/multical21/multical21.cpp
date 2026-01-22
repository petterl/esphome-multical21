#include "multical21.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include <cstring>

namespace esphome {
namespace multical21 {

static const char *const TAG = "multical21";

// AES S-Box
static const uint8_t AES_SBOX[256] = {
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
    0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
    0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
    0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
    0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
    0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
    0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
    0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
    0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
    0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
    0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
    0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
    0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
    0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
    0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16};

// AES Round Constants
static const uint8_t AES_RCON[11] = {0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36};

void Multical21Component::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Multical21...");

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

  // Check GDO0 for packet available (falling edge indicates sync word detected)
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

    // Expand AES key
    memcpy(this->expanded_key_, this->aes_key_, 16);
    for (int i = 1; i <= 10; i++) {
      uint8_t temp[4];
      memcpy(temp, &this->expanded_key_[(i - 1) * 16 + 12], 4);

      // RotWord
      uint8_t t = temp[0];
      temp[0] = temp[1];
      temp[1] = temp[2];
      temp[2] = temp[3];
      temp[3] = t;

      // SubWord
      for (int j = 0; j < 4; j++) {
        temp[j] = AES_SBOX[temp[j]];
      }

      // XOR with Rcon
      temp[0] ^= AES_RCON[i];

      for (int j = 0; j < 4; j++) {
        this->expanded_key_[i * 16 + j] = this->expanded_key_[(i - 1) * 16 + j] ^ temp[j];
      }
      for (int j = 4; j < 16; j++) {
        this->expanded_key_[i * 16 + j] = this->expanded_key_[(i - 1) * 16 + j] ^ this->expanded_key_[i * 16 + j - 4];
      }
    }

    ESP_LOGD(TAG, "AES key set and expanded");
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

  // Log raw frame
  ESP_LOGD(TAG, "Received frame (%d bytes)", length);

  // Check meter ID
  if (!this->check_meter_id(this->frame_buffer_)) {
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

  // Cipher starts at index 16, and we need to subtract 2 for CRC bytes already removed
  // Original: cipherLength = length - 2 - 16
  // But in our case, length already excludes the 2-byte preamble read separately
  // So cipher_length = length - 16 (payload starts after length byte)
  uint8_t cipher_length = length - 16;
  if (cipher_length > MAX_FRAME_LENGTH - 16 || cipher_length == 0) {
    return false;
  }

  // Build IV from payload (wM-Bus specific IV construction)
  uint8_t iv[16] = {0};
  memcpy(iv, &payload[1], 8);      // bytes 1-8
  iv[8] = payload[10];             // byte 10
  memcpy(&iv[9], &payload[12], 4); // bytes 12-15 (4 bytes)
  // Rest is padded with zeros

  // Decrypt using AES-128 CTR mode
  this->aes_ctr_decrypt(&payload[16], this->plaintext_, cipher_length, iv);

  // Parse decrypted data
  this->parse_meter_data(this->plaintext_, cipher_length);

  return true;
}

void Multical21Component::aes_encrypt_block(const uint8_t *input, uint8_t *output) {
  uint8_t state[16];
  memcpy(state, input, 16);

  // Initial round key addition
  for (int i = 0; i < 16; i++) {
    state[i] ^= this->expanded_key_[i];
  }

  // 9 main rounds
  for (int round = 1; round <= 9; round++) {
    // SubBytes
    for (int i = 0; i < 16; i++) {
      state[i] = AES_SBOX[state[i]];
    }

    // ShiftRows
    uint8_t temp = state[1];
    state[1] = state[5];
    state[5] = state[9];
    state[9] = state[13];
    state[13] = temp;

    temp = state[2];
    state[2] = state[10];
    state[10] = temp;
    temp = state[6];
    state[6] = state[14];
    state[14] = temp;

    temp = state[15];
    state[15] = state[11];
    state[11] = state[7];
    state[7] = state[3];
    state[3] = temp;

    // MixColumns
    for (int i = 0; i < 4; i++) {
      uint8_t a[4];
      uint8_t b[4];
      for (int j = 0; j < 4; j++) {
        a[j] = state[i * 4 + j];
        b[j] = (state[i * 4 + j] << 1) ^ ((state[i * 4 + j] & 0x80) ? 0x1b : 0);
      }
      state[i * 4 + 0] = b[0] ^ a[1] ^ b[1] ^ a[2] ^ a[3];
      state[i * 4 + 1] = a[0] ^ b[1] ^ a[2] ^ b[2] ^ a[3];
      state[i * 4 + 2] = a[0] ^ a[1] ^ b[2] ^ a[3] ^ b[3];
      state[i * 4 + 3] = a[0] ^ b[0] ^ a[1] ^ a[2] ^ b[3];
    }

    // AddRoundKey
    for (int i = 0; i < 16; i++) {
      state[i] ^= this->expanded_key_[round * 16 + i];
    }
  }

  // Final round (no MixColumns)
  // SubBytes
  for (int i = 0; i < 16; i++) {
    state[i] = AES_SBOX[state[i]];
  }

  // ShiftRows
  uint8_t temp = state[1];
  state[1] = state[5];
  state[5] = state[9];
  state[9] = state[13];
  state[13] = temp;

  temp = state[2];
  state[2] = state[10];
  state[10] = temp;
  temp = state[6];
  state[6] = state[14];
  state[14] = temp;

  temp = state[15];
  state[15] = state[11];
  state[11] = state[7];
  state[7] = state[3];
  state[3] = temp;

  // AddRoundKey
  for (int i = 0; i < 16; i++) {
    output[i] = state[i] ^ this->expanded_key_[160 + i];
  }
}

void Multical21Component::aes_ctr_decrypt(const uint8_t *cipher, uint8_t *plain, uint8_t length, const uint8_t *iv) {
  uint8_t counter[16];
  uint8_t keystream[16];

  memcpy(counter, iv, 16);

  for (uint8_t i = 0; i < length; i += 16) {
    // Encrypt counter to get keystream block
    this->aes_encrypt_block(counter, keystream);

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
