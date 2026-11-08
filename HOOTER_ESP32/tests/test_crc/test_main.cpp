// FILE: //tests/test_crc/test_main.cpp

// purpose: testing our CRC calculation function, crc16_ccitt, to ensure that
//it is working correctly

#include <unity.h> // test framework, PlatformIO reads its output for PASS/FAIL
#include "common_methods/crc/crc.h"
#include "stdio.h" // we need to use fprintf to print to the console
#include "stdlib.h" // we need to generate random data for the sake of testing

#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h" // for the startup delay on the ESP32
#include "freertos/task.h"
#endif

// Unity calls these before and after every test, we don't need them
//
void setUp(void) {}
void tearDown(void) {}

//=========================
// Test 1: 123456789, we expect the CRC to be 0x29B1
//=========================
static void test_crc_check_value(void){
    // Testing just basic ass values to ensure that our CRC function is working
    //
    const uint8_t test_data1[] =  {'1','2','3','4','5','6','7','8','9'};
    uint16_t crc1 = crc16_ccitt(test_data1, sizeof(test_data1));
    fprintf(stdout, "CRC of \"123456789\" = 0x%04X (expected 0x29B1)\n", crc1);
    TEST_ASSERT_EQUAL_HEX16(0x29B1, crc1);
}
//=========================
// End of Test 1
// =========================

// =========================
// Test 2: 890 bytes of random data
// =========================
static void test_crc_random_data(void){
    const size_t test_data2_len = 890;
    // initialize where we wil lstore our data
    // (static so it isn't on the task's stack)
    static uint8_t test_data2[test_data2_len];
    srand(42); // fixed seed: same "random" data every run
    for (size_t i = 0; i < test_data2_len; i++) {
        test_data2[i] = (uint8_t)(rand() % 256);   // random value 0-255
    }

    fprintf(stdout, "First 16 bytes:\n");
    for (size_t j = 0; j< 16; j++) {
        for (int bit = 7; bit >= 0; bit--) {
            fprintf(stdout, "%d", (test_data2[j] >> bit) & 1);
        }
    }
    fprintf(stdout, "\n");

    // flipping a single bit has to change the CRC, otherwise the CRC
    // wouldn't catch a corrupted packet
    uint16_t original = crc16_ccitt(test_data2, test_data2_len);
    test_data2[100] ^= 0x01; // flip one bit
    uint16_t flipped = crc16_ccitt(test_data2, test_data2_len);
    fprintf(stdout, "CRC original = 0x%04X, after bit flip = 0x%04X\n",
            original, flipped);
    TEST_ASSERT_NOT_EQUAL(original, flipped);
}
// =========================
// End of Test 2
// =========================

//=========
// run_tests: runs every CRC test through Unity, which prints the results
// input(s):
// N/A
//
// output(s):
// failures: int of failed tests
//
static int run_tests(){
    UNITY_BEGIN();
    RUN_TEST(test_crc_check_value);
    RUN_TEST(test_crc_random_data);
    return UNITY_END();   // 0 means every test passed
}

//=========
// entry point: the ESP32 (ESP-IDF) starts at app_main(), the PC at main()
//
#ifdef ESP_PLATFORM
extern "C" void app_main(void){
    vTaskDelay(pdMS_TO_TICKS(2000)); // give the serial monitor time to connect
    run_tests();
}
#else
int main(){
    return run_tests();
}
#endif