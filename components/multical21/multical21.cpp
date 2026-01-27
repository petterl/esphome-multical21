// Multical21 ESPHome Component
// Kamstrup Multical 21 water meter reader via CC1101 (wM-Bus Mode C1)
//
// Based on work by:
//   Patrik Thalin - https://github.com/pthalin/esp32-multical21
//   Chester - https://github.com/chester4444/esp-multical21

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

  // Check GDO0 for packet available (GDO0 LOW means sync detected)
  if (this->gdo0_pin_ != nullptr && !this->gdo0_pin_->digital_read()) {
    this->receive_frame();
  }
}

void Multical21Component::update() {
  // Values are published when received in loop(), nothing to do here
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

// Convert hex string to bytes using direct character arithmetic
// This is faster than strtol() as it avoids function call overhead and string allocation
void Multical21Component::hex_to_bytes(const std::string &hex, uint8_t *bytes, size_t len) {
  for (size_t i = 0; i < len && i * 2 + 1 < hex.length(); i++) {
    char hi = hex[i * 2];
    char lo = hex[i * 2 + 1];
    // Convert ASCII hex char to value: '0'-'9' -> 0-9, 'a'-'f'/'A'-'F' -> 10-15
    uint8_t hi_val = (hi >= 'a') ? (hi - 'a' + 10) : (hi >= 'A') ? (hi - 'A' + 10) : (hi - '0');
    uint8_t lo_val = (lo >= 'a') ? (lo - 'a' + 10) : (lo >= 'A') ? (lo - 'A' + 10) : (lo - '0');
    bytes[i] = (hi_val << 4) | lo_val;
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

// Read multiple bytes in a single SPI transaction (burst mode)
// More efficient than multiple read_register() calls for sequential data
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

// Reset CC1101 to receive mode
// Uses 100µs polling intervals (vs 1ms) for faster state transitions while
// maintaining same 5ms max timeout. State changes typically complete in <500µs.
void Multical21Component::start_receiver() {
  // Enter IDLE state
  this->send_strobe(CC1101_SIDLE);

  // Wait for IDLE (typically <500µs)
  for (int i = 0; i < 50; i++) {
    if ((this->read_status_register(CC1101_MARCSTATE) & 0x1F) == MARCSTATE_IDLE) {
      break;
    }
    delayMicroseconds(100);
  }

  // Flush RX FIFO
  this->send_strobe(CC1101_SFRX);

  // Enter RX state
  this->send_strobe(CC1101_SRX);

  // Wait for RX (typically <500µs)
  for (int i = 0; i < 50; i++) {
    if ((this->read_status_register(CC1101_MARCSTATE) & 0x1F) == MARCSTATE_RX) {
      break;
    }
    delayMicroseconds(100);
  }
}

bool Multical21Component::receive_frame() {
  // Read preamble (should be 0x54 0x3D)
  uint8_t preamble[2];
  preamble[0] = this->read_register(CC1101_RXFIFO);
  preamble[1] = this->read_register(CC1101_RXFIFO);

  if (preamble[0] != WMBUS_PREAMBLE_1 || preamble[1] != WMBUS_PREAMBLE_2) {
    this->start_receiver();
    return false;
  }

  // Read payload length
  uint8_t length = this->read_register(CC1101_RXFIFO);
  if (length < 18 || length >= MAX_FRAME_LENGTH) {
    ESP_LOGW(TAG, "Invalid frame length: %d", length);
    this->start_receiver();
    return false;
  }

  // Read payload using burst mode (single SPI transaction vs per-byte reads)
  // This reduces SPI overhead from O(n) transactions to O(1)
  this->read_burst(CC1101_RXFIFO, this->frame_buffer_, length);
  this->start_receiver();

  // Process only if meter ID matches
  if (!this->check_meter_id(this->frame_buffer_)) {
    return false;
  }

  return this->decrypt_frame(this->frame_buffer_, length);
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

// Decrypt wM-Bus Mode C1 encrypted payload
// Frame structure: [header 16 bytes][encrypted data][CRC 2 bytes]
bool Multical21Component::decrypt_frame(const uint8_t *payload, uint8_t length) {
  if (length < 18) {
    return false;
  }

  // Cipher length = total - 16 byte header - 2 byte CRC
  uint8_t cipher_length = length - 18;
  if (cipher_length == 0 || cipher_length > MAX_FRAME_LENGTH - 16) {
    return false;
  }

  // Build wM-Bus IV per EN 13757-4:
  // IV[0-7]  = M-field + A-field (payload bytes 1-8)
  // IV[8]    = ACC (payload byte 10)
  // IV[9-12] = SN (payload bytes 12-15)
  // IV[13-15] = 0x00 padding
  uint8_t iv[16];
  memcpy(iv, &payload[1], 8);
  iv[8] = payload[10];
  memcpy(&iv[9], &payload[12], 4);
  memset(&iv[13], 0, 3);

  this->aes_ctr_decrypt(&payload[16], this->plaintext_, cipher_length, iv);
  this->parse_meter_data(this->plaintext_, cipher_length);

  return true;
}

// AES-128 CTR mode decryption using mbedtls
// CTR mode: encrypt counter block, XOR with ciphertext to get plaintext
// Uses pointer arithmetic for efficient memory access
void Multical21Component::aes_ctr_decrypt(const uint8_t *cipher, uint8_t *plain, uint8_t length, const uint8_t *iv) {
  uint8_t counter[16];
  uint8_t keystream[16];

  memcpy(counter, iv, 16);

  uint8_t remaining = length;
  const uint8_t *src = cipher;
  uint8_t *dst = plain;

  while (remaining > 0) {
    // Encrypt counter to get keystream block (mbedtls handles AES rounds)
    mbedtls_aes_crypt_ecb(&this->aes_ctx_, MBEDTLS_AES_ENCRYPT, counter, keystream);

    // XOR ciphertext with keystream to produce plaintext
    uint8_t block_len = (remaining < 16) ? remaining : 16;
    for (uint8_t j = 0; j < block_len; j++) {
      *dst++ = *src++ ^ keystream[j];
    }
    remaining -= block_len;

    // Increment 128-bit counter (big-endian): increment from LSB, propagate carry
    for (int j = 15; j >= 0 && ++counter[j] == 0; j--) {}
  }
}

// CRC16 EN13757 lookup table for wM-Bus frames
// Polynomial: 0x3D65 (EN 13757-4), pre-computed for byte-at-a-time processing
// Using a lookup table is ~8x faster than bit-by-bit calculation
static const uint16_t CRC16_TABLE[256] = {
    0x0000, 0x3D65, 0x7ACA, 0x47AF, 0xF594, 0xC8F1, 0x8F5E, 0xB23B,
    0xD64D, 0xEB28, 0xAC87, 0x91E2, 0x23D9, 0x1EBC, 0x5913, 0x6476,
    0x91FF, 0xAC9A, 0xEB35, 0xD650, 0x646B, 0x590E, 0x1EA1, 0x23C4,
    0x47B2, 0x7AD7, 0x3D78, 0x001D, 0xB226, 0x8F43, 0xC8EC, 0xF589,
    0x1E9B, 0x23FE, 0x6451, 0x5934, 0xEB0F, 0xD66A, 0x91C5, 0xACA0,
    0xC8D6, 0xF5B3, 0xB21C, 0x8F79, 0x3D42, 0x0027, 0x4788, 0x7AED,
    0x8F64, 0xB201, 0xF5AE, 0xC8CB, 0x7AF0, 0x4795, 0x003A, 0x3D5F,
    0x5929, 0x644C, 0x23E3, 0x1E86, 0xACBD, 0x91D8, 0xD677, 0xEB12,
    0x3D36, 0x0053, 0x47FC, 0x7A99, 0xC8A2, 0xF5C7, 0xB268, 0x8F0D,
    0xEB7B, 0xD61E, 0x91B1, 0xACD4, 0x1EEF, 0x238A, 0x6425, 0x5940,
    0xACC9, 0x91AC, 0xD603, 0xEB66, 0x595D, 0x6438, 0x2397, 0x1EF2,
    0x7A84, 0x47E1, 0x004E, 0x3D2B, 0x8F10, 0xB275, 0xF5DA, 0xC8BF,
    0x23AD, 0x1EC8, 0x5967, 0x6402, 0xD639, 0xEB5C, 0xACF3, 0x9196,
    0xF5E0, 0xC885, 0x8F2A, 0xB24F, 0x0074, 0x3D11, 0x7ABE, 0x47DB,
    0xB252, 0x8F37, 0xC898, 0xF5FD, 0x47C6, 0x7AA3, 0x3D0C, 0x0069,
    0x641F, 0x597A, 0x1ED5, 0x23B0, 0x918B, 0xACEE, 0xEB41, 0xD624,
    0x7A6C, 0x4709, 0x00A6, 0x3DC3, 0x8FF8, 0xB29D, 0xF532, 0xC857,
    0xAC21, 0x9144, 0xD6EB, 0xEB8E, 0x59B5, 0x64D0, 0x237F, 0x1E1A,
    0xEB93, 0xD6F6, 0x9159, 0xAC3C, 0x1E07, 0x2362, 0x64CD, 0x59A8,
    0x3DDE, 0x00BB, 0x4714, 0x7A71, 0xC84A, 0xF52F, 0xB280, 0x8FE5,
    0x64F7, 0x5992, 0x1E3D, 0x2358, 0x9163, 0xAC06, 0xEBA9, 0xD6CC,
    0xB2BA, 0x8FDF, 0xC870, 0xF515, 0x472E, 0x7A4B, 0x3DE4, 0x0081,
    0xF508, 0xC86D, 0x8FC2, 0xB2A7, 0x009C, 0x3DF9, 0x7A56, 0x4733,
    0x2345, 0x1E20, 0x598F, 0x64EA, 0xD6D1, 0xEBB4, 0xAC1B, 0x917E,
    0x475A, 0x7A3F, 0x3D90, 0x00F5, 0xB2CE, 0x8FAB, 0xC804, 0xF561,
    0x9117, 0xAC72, 0xEBDD, 0xD6B8, 0x6483, 0x59E6, 0x1E49, 0x232C,
    0xD6A5, 0xEBC0, 0xAC6F, 0x910A, 0x2331, 0x1E54, 0x59FB, 0x649E,
    0x00E8, 0x3D8D, 0x7A22, 0x4747, 0xF57C, 0xC819, 0x8FB6, 0xB2D3,
    0x59C1, 0x64A4, 0x230B, 0x1E6E, 0xAC55, 0x9130, 0xD69F, 0xEBFA,
    0x8F8C, 0xB2E9, 0xF546, 0xC823, 0x7A18, 0x477D, 0x00D2, 0x3DB7,
    0xC83E, 0xF55B, 0xB2F4, 0x8F91, 0x3DAA, 0x00CF, 0x4760, 0x7A05,
    0x1E73, 0x2316, 0x64B9, 0x59DC, 0xEBE7, 0xD682, 0x912D, 0xAC48
};

// Calculate CRC16 using table lookup (one table access per byte)
uint16_t Multical21Component::crc16_en13757(const uint8_t *data, size_t length) {
  uint16_t crc = 0x0000;
  for (size_t i = 0; i < length; i++) {
    crc = (crc << 8) ^ CRC16_TABLE[((crc >> 8) ^ data[i]) & 0xFF];
  }
  return ~crc;  // Final XOR with 0xFFFF per EN 13757
}

void Multical21Component::parse_meter_data(const uint8_t *data, uint8_t length) {
  if (length < 3) {
    return;
  }

  // Determine frame type and positions
  int pos_total, pos_target, pos_flow_temp, pos_ambient_temp;

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

  // Verify CRC
  uint16_t calc_crc = this->crc16_en13757(data + 2, length - 2);
  uint16_t read_crc = (data[1] << 8) | data[0];

  if (calc_crc != read_crc) {
    ESP_LOGW(TAG, "CRC mismatch");
    return;
  }

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

  // Calculate and publish current flow (L/h)
  if (this->current_flow_sensor_ != nullptr) {
    uint32_t current_time = millis();
    if (this->prev_reading_time_ > 0 && this->prev_total_ > 0) {
      float delta_total_liters = (total_m3 - this->prev_total_) * 1000.0f;
      float delta_time_hours = (current_time - this->prev_reading_time_) / 3600000.0f;
      if (delta_time_hours > 0.001f && delta_total_liters >= 0) {
        float flow_lph = delta_total_liters / delta_time_hours;
        this->current_flow_sensor_->publish_state(flow_lph);
      }
    }
    this->prev_total_ = total_m3;
    this->prev_reading_time_ = current_time;
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
