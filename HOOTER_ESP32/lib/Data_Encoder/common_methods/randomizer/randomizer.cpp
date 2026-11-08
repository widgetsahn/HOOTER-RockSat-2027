// FILE: \Data_Encoder\common_methods\randomizer\randomizer.cpp

#include "randomizer.h"

// CCSDS TM randomizer: polynomial h(x) = x^8 + x^7 + x^5 + x^3 + 1
// the shift register starts at all ones at the start of every frame
static constexpr uint8_t RAND_INIT = 0xFF;
//==============================
// randomize: randomizes a given data buffer
// input(s): 
// data: pointer to the first byte of data with the with the CCSDS
//pseudo-random sequence.
// len: representing number of bytes we need to process
// 
// output(s): 
// none, the data buffer is modified in place
//
void randomize(uint8_t* data, size_t len){
    uint8_t reg = RAND_INIT;   // shift register, reset for every frame
    // loops through every byte of the data buffer
    // 
    for (size_t i = 0; i < len; i++) { 

        uint8_t byte = 0; // next byte of the sequence

        for (int b = 0; b < 8; b++) { // build it one bit at a time
            // output bit: the top bit of the register
            uint8_t out = (reg >> 7) & 1;

            // feedback bit: XOR of the tap bits 7, 4, 2 and 0
            uint8_t fb = ((reg >> 7) ^ (reg >> 4) ^ (reg >> 2) ^ reg) & 1;

            // shift the register left, feedback bit enters at the bottom
            reg = (uint8_t)((reg << 1) | fb);

            // add the output bit to the sequence byte, first bit on top
            byte = (uint8_t)((byte << 1) | out);
        }

        data[i] ^= byte;   // XOR the data byte with the sequence byte
    }
}

// ==============================
// derandomize is simply just running the data through randomize again, since 
// the way that we are randomizing the data (using XORs) is entirely
// reversible. main point of this existing instead of just calling randomize
// again is just for the sake of someone reading this code
//==============================
// derandomize: undoes the randomization of a given data buffer
// input(s): 
// data: pointer to the first byte of data
// len: size_t representing number of bytes we need to process
// 
// output(s): 
// none, the data buffer is modified in place
//
void derandomize(uint8_t* data, size_t len){
    randomize(data, len);
}