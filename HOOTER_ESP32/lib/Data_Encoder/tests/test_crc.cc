// FILE: 

// purpose: testing our CRC calculation function, crc16_ccitt, to ensure that 
//it is working correctly

#include "../common_methods/crc/crc.h"
#include "stdio.h" // we need to use fprintf to print to the console
#include "stdlib.h" // we need to generate random data for the sake of testing
//=========
// main: calculates the CRC of some test data and prints it to the console
// input(s): 
// N/A
// 
// output(s): 
// N/A
//
int main(){
    // Testing just basic ass values to ensure that our CRC function is working
    //
    int failures = 0; // counter for failed tests

    //=========================
    // Test 1: 123456789, we expect the CRC to be 0x29B1
    //==========
    const uint8_t test_data1[] =  {'1','2','3','4','5','6','7','8','9'};
    uint16_t crc1 = crc16_ccitt(test_data1, sizeof(test_data1));
     fprintf
     (stdout,"CRC of \"123456789\" = 0x%04X (expected 0x29B1) %s\n",
           crc1, crc1 == 0x29B1 ? "PASS" : "FAIL");
    if (crc1 != 0x29B1) failures++;
    //=========================
    // End of Test 1
    // =========================

    // =========================
    // Test 2: 890 bytes of random data
    // =========================
    const size_t test_data2_len = 890;
    // initialize where we wil lstore our data

    uint8_t test_data2[test_data2_len];
    ; // fixed seed: same "random" data every run
    for (size_t i = 0; i < 890; i++) {
    test_data2[i] = (uint8_t)(rand() % 256);   // random value 0-255
    }

    fprintf(stdout, "First 16 bytes:\n");
    for (size_t j = 0; j< 16; j++) {
        for (int bit = 7; bit >= 0; bit--) {
            fprintf(stdout, "%d", (test_data2[j] >> bit) & 1);
    }
    fprintf(stdout, "\n");
    }
    return failures;   // 0 means every test passed
}
