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
//=========================
static void print_bytes(const char* label, const uint8_t* buf, size_t n){
    printf("%s", label);
    for (size_t i = 0; i < n; i++) {
        printf("%02X ", buf[i]);
    }
    printf("\n");
}

#ifdef ESP_PLATFORM
// ===================================================================
// ESP32 SIDE: Unity tests, then randomize a frame and print it
// ===================================================================

// buffer the ESP32 randomizes (static: not on the task's stack)
static uint8_t frame[RAND_LEN];

// Unity calls these before and after every test, we don't need them
void setUp(void) {}
void tearDown(void) {}

//=========================
// Test 1: known CCSDS sequence
// randomizing zeros leaves just the sequence itself (0 XOR x = x), so the
// first 16 bytes must match the published CCSDS TM sequence
//=========================
static void test_known_sequence(void){
    const uint8_t expected[16] = {
        0xFF, 0x48, 0x0E, 0xC0, 0x9A, 0x0D, 0x70, 0xBC,
        0x8E, 0x2C, 0x93, 0xAD, 0xA7, 0xB7, 0x46, 0xCE
    };
    memset(frame, 0, RAND_LEN);
    randomize(frame, RAND_LEN);
    print_bytes("Sequence, first 16 bytes: ", frame, 16);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, frame, 16);
}

//=========================
// Test 2: randomizing a full frame actually changes the data
// (derandomizing is checked on the PC side)
//=========================
static void test_full_frame_changes(void){
    make_test_data(original, RAND_LEN);
    memcpy(frame, original, RAND_LEN);
    randomize(frame, RAND_LEN);
    TEST_ASSERT_FALSE(memcmp(original, frame, RAND_LEN) == 0);
}

//=========================
// Test 3: worst case, a frame of all zeros (e.g. sensors reading 0).
// Before randomizing that's 8160 identical bits in a row; afterward the
// longest run of identical bits must be short so the receiver stays locked
//=========================
static void test_breaks_up_long_runs(void){
    memset(frame, 0, RAND_LEN);
    randomize(frame, RAND_LEN);

    int longest = 1, current = 1;
    int prev = (frame[0] >> 7) & 1;
    for (size_t i = 0; i < RAND_LEN; i++) {
        for (int bit = (i == 0 ? 6 : 7); bit >= 0; bit--) {
            int b = (frame[i] >> bit) & 1;
            current = (b == prev) ? current + 1 : 1;
            if (current > longest) longest = current;
            prev = b;
        }
    }
    printf("Longest run of identical bits: %d (was %u before)\n",
           longest, (unsigned)(RAND_LEN * 8));
    TEST_ASSERT_LESS_OR_EQUAL(8, longest);
}

extern "C" void app_main(void){
    vTaskDelay(pdMS_TO_TICKS(2000)); // give the test runner time to connect

    UNITY_BEGIN();
    RUN_TEST(test_known_sequence);
    RUN_TEST(test_full_frame_changes);
    RUN_TEST(test_breaks_up_long_runs);

    // randomize one frame of test data and print it on a single line.
    // printed BEFORE UNITY_END(): PlatformIO stops reading serial once it
    // sees Unity's final summary line
    make_test_data(frame, RAND_LEN);
    randomize(frame, RAND_LEN);
    print_bytes("RANDOMIZED: ", frame, RAND_LEN);
    fflush(stdout);

    UNITY_END();
}

#else
// ===================================================================
// PC SIDE: read the randomized bytes and derandomize them
// ===================================================================

static uint8_t received[RAND_LEN];

int main(int argc, char* argv[]){
    if (argc < 2) {
        printf("usage: %s randomized.txt [derandomized.txt]\n", argv[0]);
        return 1;
    }

    FILE* in = fopen(argv[1], "r");
    if (in == NULL) {
        printf("could not open %s\n", argv[1]);
        return 1;
    }

    // read hex bytes ("B9 2C 3F ...") until the file ends or the frame is
    // full. spaces and line breaks between bytes are fine.
    size_t count = 0;
    unsigned int value;
    while (count < RAND_LEN && fscanf(in, "%2x", &value) == 1) {
        received[count++] = (uint8_t)value;
    }
    fclose(in);

    if (count != RAND_LEN) {
        printf("expected %u bytes in %s, found %u\n",
               (unsigned)RAND_LEN, argv[1], (unsigned)count);
        return 1;
    }

    make_test_data(original, RAND_LEN);   // the same data the ESP32 used

    print_bytes("Randomized (ESP32), first 16: ", received, 16);
    derandomize(received, RAND_LEN);      // undo the ESP32's randomizing
    print_bytes("Derandomized (PC),  first 16: ", received, 16);
    print_bytes("Expected,           first 16: ", original, 16);

    // compare every byte and report the first mismatch, if any
    size_t mismatches = 0;
    for (size_t i = 0; i < RAND_LEN; i++) {
        if (received[i] != original[i]) {
            if (mismatches == 0) {
                printf("first mismatch at byte %u: got %02X, expected %02X\n",
                       (unsigned)i, received[i], original[i]);
            }
            mismatches++;
        }
    }

    // optionally save the derandomized bytes, 16 per line
    if (argc >= 3) {
        FILE* out = fopen(argv[2], "w");
        if (out != NULL) {
            for (size_t i = 0; i < RAND_LEN; i++) {
                fprintf(out, "%02X%s", received[i],
                        (i + 1) % 16 == 0 ? "\n" : " ");
            }
            fprintf(out, "\n");
            fclose(out);
            printf("derandomized bytes saved to %s\n", argv[2]);
        }
    }

    if (mismatches == 0) {
        printf("PASS: all %u bytes match the original test data\n",
               (unsigned)RAND_LEN);
        return 0;
    }
    printf("FAIL: %u of %u bytes differ\n",
           (unsigned)mismatches, (unsigned)RAND_LEN);
    return 1;
}
#endif