// FILE: \rocksat_software\HOOTER-RockSat-2027\HOOTER_ESP32\lib\Data_Encoder\encoding_functions.h
// system include files
//

#ifndef ENCODING_FUNCTIONS_H
#define ENCODING_FUNCTIONS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <iostream>


class encoding_functions
{
public:
    // --- The Baseline / Simple Block Codes ---
    // 
    // the following are the most common block codes used in spaceflight applications. They are all linear block codes, meaning that they can be represented by a generator matrix and a parity check matrix. The generator matrix is used to encode the data, while the parity check matrix is used to decode the data and correct errors.
    //
    unsigned long encode_hamming(const unsigned int* in, unsigned long in_len,
    unsigned int* out); 
    // 
    unsigned long encode_golay_24_12(const unsigned int* in, unsigned long in_len, unsigned int* out);
    // block_length:              n = 2^m - 1, from 7 to 1023
    // error_correction_capacity: t, bit errors corrected per block
    // in_bits:                   k data bits (k is the return value)
    // out_bits:                  buffer of at least n bits
    // returns: k (data bits per block)
    
    int encode_bch(int block_length, int error_correction_capacity, const unsigned int* in_bits, unsigned int out_bits);

    // --- Continuous / Stream Codes ---
    int encode_convolutional(float rate, int constraint_length, const unsigned int* in_bits, unsigned int* out_bits, unsigned long out_cap_bits);

    // --- Burst Error Codes ---
    int encode_reed_solomon(const uint8_t* in, size_t len, unsigned int* out, int interleave = 1);
    
    // --- The Spaceflight Standards (CCSDS) ---
    void encode_concatenated(); // RS (Outer) + Convolutional (Inner)
    void encode_ldpc(float rate);
    
    // --- Error Detection (Framing) ---
    uint32_t calculate_crc32(unsigned char* data, int len);
};
// end of include file
//
#endif