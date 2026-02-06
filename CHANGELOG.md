# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2026-02-06

### Added
- Full ESPHome external component for Kamstrup Multical 21 water meters
- CC1101 radio module support for wM-Bus Mode C1 (868.95 MHz)
- AES-128 CTR decryption using mbedtls
- CRC16 EN13757 validation with lookup table
- Sensors: total consumption, month start value, water temperature, ambient temperature, current flow
- Diagnostic sensors: frames received, CRC errors, signal quality
- Text sensor: last update counter with uptime
- Input validation for meter ID (8 hex chars) and AES key (32 hex chars)
- Support for compact frames (CI=0x79) and long frames (CI=0x78)
- Example configurations for ESP32, ESP32-C3 Super Mini, and ESP8266 (D1 Mini)
- CI workflow with Python linting, YAML linting, and unit tests
- GitHub Actions release workflow with version verification
- Version constant with boot logging
- Semantic versioning via git tags

[1.0.0]: https://github.com/petterl/esp32-multical21/releases/tag/v1.0.0
