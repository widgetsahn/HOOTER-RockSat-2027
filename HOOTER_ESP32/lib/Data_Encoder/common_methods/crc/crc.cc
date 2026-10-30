// FILE: \Data_Encoder\common_methods\crc\crc.cc

#include "crc.h"

#ifdef ESP_PLATFORM
#include "esp_rom_crc.h" // we need the ESP32's built-in CRC function, but
// only if we are compiling for the ESP32
#endif

// defining generator polynomial for CRC calculation x^16 + x^12 + x^5 + 1
static constexpr uint16_t CRC16_POLY = 0x1021;
static constexpr uint16_t CRC16_INIT = 0xFFFF;

uint16_t crc16_ccitt(const uint8_t* data, size_t len){
#ifdef ESP_PLATFORM
    // ROM version. The ROM function inverts the value on the way in and on
    // the way out, so we pre-invert the start value and invert the result
    // to get the standard CRC-16-CCITT (init 0xFFFF, no final XOR).
    return (uint16_t)~esp_rom_crc16_be((uint16_t)~CRC16_INIT, data, len);
#else
    // portable version: long division one bit at a time
    uint16_t crc = CRC16_INIT;

    for (size_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;       // bring the next byte into the top 8 bits

        for (int bit = 0; bit < 8; bit++) {
            if (crc & 0x8000) {              // top bit is 1: "divisor fits"
                crc = (crc << 1) ^ CRC16_POLY;  // shift and subtract (XOR) the divisor
            } else {
                crc = crc << 1;              // top bit is 0: just shift
            }
        }
    }
    return crc;                              // the remainder is the CRC
#endif
}