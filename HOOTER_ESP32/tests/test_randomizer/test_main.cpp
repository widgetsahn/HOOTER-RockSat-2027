// FILE: tests/test_randomizer/test_main.cpp

// purpose: testing our randomizer on a full-size frame: the 1020 bytes that
// follow the sync marker (FRAME_LEN - SYNC_LEN)
//
// ESP32 side (pio test -e esp32dev -f test_randomizer -v):
//   1) runs the Unity tests below (randomizing only, like in flight)
//   2) randomizes one frame of test data and prints it on one line:
//        RANDOMIZED: B9 2C 3F ...  (1020 bytes in hex)
//
// PC side (compiled with g++ in MSYS2):
//   ./test_randomizer.exe randomized.txt [derandomized.txt]
//   reads the randomized bytes, derandomizes them and checks they match the
//   test data. Both sides build the same test data with make_test_data(),
//   so the file only needs to hold the randomized bytes.

#include <stdio.h>  // printf, fopen
#include <stdlib.h> // strtoul
#include <string.h> // memcpy, memset, memcmp
#include "frame_config.h"
#include "common_methods/randomizer/randomizer.h"

#ifdef ESP_PLATFORM
#include <unity.h>             // test framework (ESP32 side only)
#include "freertos/FreeRTOS.h" // for vTaskDelay
#include "freertos/task.h"
#endif

// number of bytes randomized in one frame: everything after the sync marker
static constexpr size_t RAND_LEN = FRAME_LEN - SYNC_LEN;  // 1020 bytes

// test data, used by both sides. static so it isn't on the task's stack
static uint8_t original[RAND_LEN];

//=========================
// make_test_data: fills buf with pseudo-random test bytes.
// rand() gives different numbers on the ESP32 and the PC, so this uses its
// own simple generator: both sides get exactly the same bytes, and the PC
// can rebuild the ESP32's original data without it being sent.
//=========================
static void make_test_data(uint8_t* buf, size_t n){
    uint32_t x = 42;                        // fixed starting value (seed)
    for (size_t i = 0; i < n; i++) {
        x = x * 1103515245u + 12345u;       // standard linear congruential step
        buf[i] = (uint8_t)(x >> 16);        // use middle bits as the byte
    }
}

//=========================
// print_bytes: prints a label and the first n bytes of a buffer in hex
//================