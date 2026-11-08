//FILE: \tests\test_interleaver\test_main.cpp

// purpose: testing our interleaver on a full set of Reed-Solomon blocks:
// INTERLEAVE_DEPTH blocks of RS_N bytes each (4 x 255 = 1020 bytes)
//
// ESP32 side (pio test -e esp32dev -f test_interleaver -v):
//   1) runs the Unity tests below (interleaving only, like in flight)
//   2) interleaves one set of test blocks and prints it on one line:
//        INTERLEAVED: 89 ... (1020 bytes in hex)
//
// PC side (compiled with g++ in MSYS2):
//   ./test_interleaver.exe interleaved.txt [deinterleaved.txt]
//   reads the interleaved bytes, deinterleaves them and checks they match
//   the test data. Both sides build the same test data with
//   make_test_data(), so the file only needs to hold the interleaved bytes.

#include <stdio.h>  // printf, fopen, fscanf
#include <string.h> // memcpy, memset
#include "common_methods/interleaver/interleaver.h"
#include "frame_config.h" // number of blocks (INTERLEAVE_DEPTH), block size (RS_N)

#ifdef ESP_PLATFORM
#include "unity.h"             // Unity test framework (ESP32 side only)
#include "freertos/FreeRTOS.h" // for vTaskDelay
#include "freertos/task.h"
#endif

// total bytes in one interleaved set: every block, one after another
static constexpr size_t IL_LEN = RS_N * INTERLEAVE_DEPTH;  // 1020 bytes

// test data in block order, used by both sides. static so it isn't on the
// task's stack
static uint8_t original[IL_LEN];

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
// ESP32 SIDE: Unity tests, then interleave a set of blocks and print it
// ===================================================================

// interleaved output (static: not on the task's stack)
static uint8_t woven[IL_LEN];

// Unity calls these before and after every test, we don't need them
void setUp(void) {}
void tearDown(void) {}

//=========================
// fill_with_block_ids: fills each block with its own ID so the interleaved
// output is easy to read: block A = 0x0A, B = 0x0B, C = 0x0C, D = 0x0D, ...
//=========================
static void fill_with_block_ids(uint8_t* buf){
    for (size_t k = 0; k < INTERLEAVE_DEPTH; k++) {
        memset(&buf[k * RS_N], (int)(0x0A + k), RS_N);
    }
}

//=========================
// Test 1: block pattern
// interleaving blocks filled with their IDs must give the repeating
// pattern 0A 0B 0C 0D 0A 0B 0C 0D ... all the way through
//=========================
static void test_block_pattern(void){
    static uint8_t blocks[IL_LEN];
    fill_with_block_ids(blocks);
    interleave(blocks, woven);

    print_bytes("Interleaved block IDs, first 12: ", woven, 12);
    for (size_t pos = 0; pos < IL_LEN; pos++) {
        uint8_t expected = (uint8_t)(0x0A + pos % INTERLEAVE_DEPTH);
        TEST_ASSERT_EQUAL_HEX8(expected, woven[pos]);
    }
}

//=========================
// Test 2: positions
// every byte must land where the CCSDS layout says: byte j of block k goes
// to position j * INTERLEAVE_DEPTH + k
//=========================
static void test_positions(void){
    make_test_data(original, IL_LEN);
    interleave(original, woven);

    for (size_t k = 0; k < INTERLEAVE_DEPTH; k++) {
        for (size_t j = 0; j < RS_N; j++) {
            TEST_ASSERT_EQUAL_HEX8(original[k * RS_N + j],
                                   woven[j * INTERLEAVE_DEPTH + k]);
        }
    }
}

//=========================
// Test 3: burst spreading
// a 40-byte burst on the air (8 ms at 40 kbps) must hit every block about
// equally, and no block may get more than the 16 bytes RS can fix
//=========================
static void test_burst_spreading(void){
    static uint8_t blocks[IL_LEN];
    fill_with_block_ids(blocks);
    interleave(blocks, woven);

    const size_t burst_start = 100, burst_len = 40;
    size_t hits[INTERLEAVE_DEPTH] = {0};

    // each damaged byte still carries the ID of the block it came from
    for (size_t pos = burst_start; pos < burst_start + burst_len; pos++) {
        hits[woven[pos] - 0x0A]++;
    }

    for (size_t k = 0; k < INTERLEAVE_DEPTH; k++) {
        printf("40-byte burst: block %c loses %u bytes\n",
               (char)('A' + k), (unsigned)hits[k]);
        TEST_ASSERT_LESS_OR_EQUAL(16, hits[k]);
    }
}

extern "C" void app_main(void){
    vTaskDelay(pdMS_TO_TICKS(2000)); // give the test runner time to connect

    UNITY_BEGIN();
    RUN_TEST(test_block_pattern);
    RUN_TEST(test_positions);
    RUN_TEST(test_burst_spreading);

    // interleave one set of test blocks and print it on a single line.
    // printed BEFORE UNITY_END(): PlatformIO stops reading serial once it
    // sees Unity's final summary line
    make_test_data(original, IL_LEN);
    interleave(original, woven);
    print_bytes("INTERLEAVED: ", woven, IL_LEN);
    fflush(stdout);

    UNITY_END();
}

#else
// ===================================================================
// PC SIDE: read the interleaved bytes and deinterleave them
// ===================================================================

static uint8_t received[IL_LEN];
static uint8_t restored[IL_LEN];

int main(int argc, char* argv[]){
    if (argc < 2) {
        printf("usage: %s interleaved.txt [deinterleaved.txt]\n", argv[0]);
        return 1;
    }

    FILE* in = fopen(argv[1], "r");
    if (in == NULL) {
        printf("could not open %s\n", argv[1]);
        return 1;
    }

    // read hex bytes ("89 89 A5 ...") until the file ends or the set is
    // full. spaces and line breaks between bytes are fine.
    size_t count = 0;
    unsigned int value;
    while (count < IL_LEN && fscanf(in, "%2x", &value) == 1) {
        received[count++] = (uint8_t)value;
    }
    fclose(in);

    if (count != IL_LEN) {
        printf("expected %u bytes in %s, found %u\n",
               (unsigned)IL_LEN, argv[1], (unsigned)count);
        return 1;
    }

    make_test_data(original, IL_LEN);     // the same data the ESP32 used

    deinterleave(received, restored);     // undo the ESP32's interleaving

    print_bytes("Interleaved (ESP32),   first 8: ", received, 8);
    // show the start of each block so the reordering is visible
    for (size_t k = 0; k < INTERLEAVE_DEPTH; k++) {
        printf("Block %c: deinterleaved ", (char)('A' + k));
        print_bytes("", &restored[k * RS_N], 4);
        printf("         expected      ");
        print_bytes("", &original[k * RS_N], 4);
    }

    // compare every byte and report the first mismatch, if any
    size_t mismatches = 0;
    for (size_t i = 0; i < IL_LEN; i++) {
        if (restored[i] != original[i]) {
            if (mismatches == 0) {
                printf("first mismatch at byte %u (block %c, byte %u): "
                       "got %02X, expected %02X\n",
                       (unsigned)i, (char)('A' + i / RS_N),
                       (unsigned)(i % RS_N), restored[i], original[i]);
            }
            mismatches++;
        }
    }

    // optionally save the deinterleaved bytes, 16 per line
    if (argc >= 3) {
        FILE* out = fopen(argv[2], "w");
        if (out != NULL) {
            for (size_t i = 0; i < IL_LEN; i++) {
                fprintf(out, "%02X%s", restored[i],
                        (i + 1) % 16 == 0 ? "\n" : " ");
            }
            fprintf(out, "\n");
            fclose(out);
            printf("deinterleaved bytes saved to %s\n", argv[2]);
        }
    }

    if (mismatches == 0) {
        printf("PASS: all %u bytes back in their original blocks\n",
               (unsigned)IL_LEN);
        return 0;
    }
    printf("FAIL: %u of %u bytes differ\n",
           (unsigned)mismatches, (unsigned)IL_LEN);
    return 1;
}
#endif