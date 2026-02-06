"""Unit tests for Multical21 component logic.

Tests CRC16 EN13757, hex-to-bytes conversion, and frame structure constants.
These mirror the C++ implementations to catch regressions.
"""

import struct
import pytest

# ---------------------------------------------------------------------------
# CRC16 EN13757 – Python reference implementation matching the C++ lookup table
# ---------------------------------------------------------------------------

CRC16_TABLE = [
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
    0x1E73, 0x2316, 0x64B9, 0x59DC, 0xEBE7, 0xD682, 0x912D, 0xAC48,
]


def crc16_en13757(data: bytes) -> int:
    """CRC16 EN13757 matching the C++ implementation."""
    crc = 0x0000
    for byte in data:
        crc = ((crc << 8) ^ CRC16_TABLE[((crc >> 8) ^ byte) & 0xFF]) & 0xFFFF
    return crc ^ 0xFFFF


def crc16_en13757_bitwise(data: bytes) -> int:
    """Bitwise CRC16 EN13757 reference — used to validate the lookup table."""
    poly = 0x3D65
    crc = 0x0000
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            if crc & 0x8000:
                crc = ((crc << 1) ^ poly) & 0xFFFF
            else:
                crc = (crc << 1) & 0xFFFF
    return crc ^ 0xFFFF


# ---------------------------------------------------------------------------
# hex_to_bytes – Python equivalent of the C++ implementation
# ---------------------------------------------------------------------------

def hex_to_bytes(hex_str: str, length: int) -> bytes:
    """Mirror the C++ hex_to_bytes logic (character arithmetic)."""
    result = bytearray(length)
    for i in range(length):
        if i * 2 + 1 >= len(hex_str):
            break
        hi = hex_str[i * 2]
        lo = hex_str[i * 2 + 1]

        def char_val(c):
            if c >= 'a':
                return ord(c) - ord('a') + 10
            elif c >= 'A':
                return ord(c) - ord('A') + 10
            else:
                return ord(c) - ord('0')

        result[i] = (char_val(hi) << 4) | char_val(lo)
    return bytes(result)


# ---------------------------------------------------------------------------
# Frame constants (must match multical21.h)
# ---------------------------------------------------------------------------

COMPACT_FRAME_TYPE = 0x79
COMPACT_POS_TOTAL = 9
COMPACT_POS_TARGET = 13
COMPACT_POS_FLOW_TEMP = 17
COMPACT_POS_AMBIENT_TEMP = 18
COMPACT_MIN_LENGTH = 19

LONG_FRAME_TYPE = 0x78
LONG_POS_TOTAL = 10
LONG_POS_TARGET = 16
LONG_POS_FLOW_TEMP = 23
LONG_POS_AMBIENT_TEMP = 29
LONG_MIN_LENGTH = 30


# ===========================================================================
# Tests
# ===========================================================================


class TestCRC16:
    """Verify CRC16 EN13757 table and calculation."""

    def test_table_matches_bitwise(self):
        """Every table entry should match the bitwise calculation for that byte."""
        for i in range(256):
            expected = crc16_en13757_bitwise(bytes([i]))
            table_val = crc16_en13757(bytes([i]))
            assert table_val == expected, f"Mismatch for byte 0x{i:02X}: table={table_val:#06x} bitwise={expected:#06x}"

    def test_table_length(self):
        assert len(CRC16_TABLE) == 256

    def test_known_vector_empty(self):
        """CRC of empty data should be 0xFFFF (just the final XOR)."""
        assert crc16_en13757(b"") == 0xFFFF

    def test_known_vector_single_byte(self):
        """Verify a single byte CRC is deterministic and both implementations agree."""
        for val in [0x00, 0x01, 0x55, 0xAA, 0xFF]:
            assert crc16_en13757(bytes([val])) == crc16_en13757_bitwise(bytes([val]))

    def test_multi_byte_consistency(self):
        """Table-based and bitwise must agree for multi-byte sequences."""
        test_data = [
            b"\x01\x02\x03\x04",
            b"\xFF\x00\xFF\x00",
            b"\x79" + b"\x00" * 18,  # Compact-frame-sized payload
            b"\x78" + b"\x00" * 29,  # Long-frame-sized payload
            bytes(range(256)),
        ]
        for data in test_data:
            assert crc16_en13757(data) == crc16_en13757_bitwise(data), f"Mismatch for {data[:8].hex()}..."

    def test_crc_detects_bit_flip(self):
        """Flipping a single bit must change the CRC."""
        data = b"\x01\x02\x03\x04\x05"
        original_crc = crc16_en13757(data)
        flipped = bytearray(data)
        flipped[2] ^= 0x01
        assert crc16_en13757(bytes(flipped)) != original_crc


class TestHexToBytes:
    """Verify hex string parsing mirrors the C++ implementation."""

    def test_lowercase(self):
        assert hex_to_bytes("aabbccdd", 4) == bytes([0xAA, 0xBB, 0xCC, 0xDD])

    def test_uppercase(self):
        assert hex_to_bytes("AABBCCDD", 4) == bytes([0xAA, 0xBB, 0xCC, 0xDD])

    def test_mixed_case(self):
        assert hex_to_bytes("aAbBcCdD", 4) == bytes([0xAA, 0xBB, 0xCC, 0xDD])

    def test_meter_id(self):
        """Typical 8-char meter ID."""
        assert hex_to_bytes("12345678", 4) == bytes([0x12, 0x34, 0x56, 0x78])

    def test_aes_key(self):
        """Full 32-char AES key."""
        key_hex = "00112233445566778899AABBCCDDEEFF"
        result = hex_to_bytes(key_hex, 16)
        assert len(result) == 16
        assert result[0] == 0x00
        assert result[15] == 0xFF

    def test_short_string(self):
        """When hex string is shorter than requested length, remaining bytes stay 0."""
        result = hex_to_bytes("AB", 4)
        assert result[0] == 0xAB
        assert result[1] == 0x00

    def test_zeros(self):
        assert hex_to_bytes("00000000", 4) == bytes(4)


class TestFrameConstants:
    """Verify frame layout constants are self-consistent."""

    def test_compact_positions_within_min_length(self):
        """All compact frame field positions must fit within COMPACT_MIN_LENGTH."""
        assert COMPACT_POS_TOTAL + 4 <= COMPACT_MIN_LENGTH
        assert COMPACT_POS_TARGET + 4 <= COMPACT_MIN_LENGTH
        assert COMPACT_POS_FLOW_TEMP < COMPACT_MIN_LENGTH
        assert COMPACT_POS_AMBIENT_TEMP < COMPACT_MIN_LENGTH

    def test_long_positions_within_min_length(self):
        """All long frame field positions must fit within LONG_MIN_LENGTH."""
        assert LONG_POS_TOTAL + 4 <= LONG_MIN_LENGTH
        assert LONG_POS_TARGET + 4 <= LONG_MIN_LENGTH
        assert LONG_POS_FLOW_TEMP < LONG_MIN_LENGTH
        assert LONG_POS_AMBIENT_TEMP < LONG_MIN_LENGTH

    def test_frame_types_differ(self):
        assert COMPACT_FRAME_TYPE != LONG_FRAME_TYPE

    def test_long_frame_is_larger(self):
        assert LONG_MIN_LENGTH > COMPACT_MIN_LENGTH

    def test_field_ordering(self):
        """Fields should appear in order: total < target < flow_temp < ambient_temp."""
        assert COMPACT_POS_TOTAL < COMPACT_POS_TARGET < COMPACT_POS_FLOW_TEMP < COMPACT_POS_AMBIENT_TEMP
        assert LONG_POS_TOTAL < LONG_POS_TARGET < LONG_POS_FLOW_TEMP < LONG_POS_AMBIENT_TEMP


class TestFrameParsing:
    """Verify that field extraction from mock frames produces expected values."""

    @staticmethod
    def _build_compact_frame(total_liters: int, target_liters: int, flow_temp: int, ambient_temp: int) -> bytes:
        """Build a minimal compact frame payload (no real encryption/CRC)."""
        buf = bytearray(COMPACT_MIN_LENGTH)
        buf[2] = COMPACT_FRAME_TYPE
        struct.pack_into("<I", buf, COMPACT_POS_TOTAL, total_liters)
        struct.pack_into("<I", buf, COMPACT_POS_TARGET, target_liters)
        buf[COMPACT_POS_FLOW_TEMP] = flow_temp
        buf[COMPACT_POS_AMBIENT_TEMP] = ambient_temp
        # Store CRC at positions 0-1
        crc = crc16_en13757(bytes(buf[2:]))
        buf[0] = crc & 0xFF
        buf[1] = (crc >> 8) & 0xFF
        return bytes(buf)

    @staticmethod
    def _build_long_frame(total_liters: int, target_liters: int, flow_temp: int, ambient_temp: int) -> bytes:
        """Build a minimal long frame payload (no real encryption/CRC)."""
        buf = bytearray(LONG_MIN_LENGTH)
        buf[2] = LONG_FRAME_TYPE
        struct.pack_into("<I", buf, LONG_POS_TOTAL, total_liters)
        struct.pack_into("<I", buf, LONG_POS_TARGET, target_liters)
        buf[LONG_POS_FLOW_TEMP] = flow_temp
        buf[LONG_POS_AMBIENT_TEMP] = ambient_temp
        crc = crc16_en13757(bytes(buf[2:]))
        buf[0] = crc & 0xFF
        buf[1] = (crc >> 8) & 0xFF
        return bytes(buf)

    def test_compact_frame_roundtrip(self):
        """Build a compact frame and extract the values back."""
        frame = self._build_compact_frame(123456, 100000, 25, 20)
        assert frame[2] == COMPACT_FRAME_TYPE
        total = struct.unpack_from("<I", frame, COMPACT_POS_TOTAL)[0]
        target = struct.unpack_from("<I", frame, COMPACT_POS_TARGET)[0]
        assert total == 123456
        assert target == 100000
        assert frame[COMPACT_POS_FLOW_TEMP] == 25
        assert frame[COMPACT_POS_AMBIENT_TEMP] == 20

    def test_long_frame_roundtrip(self):
        """Build a long frame and extract the values back."""
        frame = self._build_long_frame(999999, 500000, 30, 22)
        assert frame[2] == LONG_FRAME_TYPE
        total = struct.unpack_from("<I", frame, LONG_POS_TOTAL)[0]
        target = struct.unpack_from("<I", frame, LONG_POS_TARGET)[0]
        assert total == 999999
        assert target == 500000
        assert frame[LONG_POS_FLOW_TEMP] == 30
        assert frame[LONG_POS_AMBIENT_TEMP] == 22

    def test_compact_frame_crc_valid(self):
        """CRC stored in frame should match calculated CRC."""
        frame = self._build_compact_frame(12345, 10000, 18, 15)
        read_crc = (frame[1] << 8) | frame[0]
        calc_crc = crc16_en13757(frame[2:])
        assert read_crc == calc_crc

    def test_long_frame_crc_valid(self):
        """CRC stored in frame should match calculated CRC."""
        frame = self._build_long_frame(12345, 10000, 18, 15)
        read_crc = (frame[1] << 8) | frame[0]
        calc_crc = crc16_en13757(frame[2:])
        assert read_crc == calc_crc

    def test_compact_frame_crc_detects_corruption(self):
        """Corrupting a byte should make CRC fail."""
        frame = bytearray(self._build_compact_frame(12345, 10000, 18, 15))
        frame[COMPACT_POS_TOTAL] ^= 0x01  # flip a bit
        read_crc = (frame[1] << 8) | frame[0]
        calc_crc = crc16_en13757(bytes(frame[2:]))
        assert read_crc != calc_crc

    def test_total_m3_conversion(self):
        """Verify raw liters -> m3 conversion (divide by 1000)."""
        raw = 123456  # liters
        m3 = raw / 1000.0
        assert abs(m3 - 123.456) < 0.0001
