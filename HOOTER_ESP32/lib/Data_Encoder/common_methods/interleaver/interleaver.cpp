//FILE: \Data_Encoder\common_methods\interleaver\interleaver.cpp

#include "interleaver.h"
#include "frame_config.h" // we need this to get the number of blocks
//(INTERLEAVE_DEPTH)and the  size of each block (RS_N) 

//==============================
// interleave: weaves together data from multiple blocks
// input(s): 
// in: pointer to the first byte of data that we want to interleave
// out: pointer to the first byte of data that we want to write the 
// interleaved data to
// 
// output(s): 
// none, the data buffer is modified in place
//

void interleave(const uint8_t* in, uint8_t* out) {
    size_t pos = 0; // next spot in the output
    // loops through every byte of the blocks
    //
    for (size_t j = 0; j < RS_N; j++) { 
        // cycles between the blocks, taking one byte from each block in turn
        //
        for (size_t k = 0; k < INTERLEAVE_DEPTH; k++) {
            out[pos++] = in[k * RS_N + j]; // byte j of block k
        }
    }
}

//==============================
// deinterleave: unweaves data from an interleaved format back into its
// original block format
// input(s): 
// in: pointer to the first byte of data that we want to deinterleave
// out: pointer to the first byte of data that we want to write the 
// deinterleaved data to
// 
// output(s): 
// none, the data buffer is modified in place
//

void deinterleave(const uint8_t* in, uint8_t* out) {
    size_t pos = 0; // next spot in the input
    // loops through every byte of the blocks
    //
    for (size_t j = 0; j < RS_N; j++) {
        // cycles between the blocks, taking one byte from each block in turn
        //
        for (size_t k = 0; k < INTERLEAVE_DEPTH; k++) {
            out[k * RS_N + j] = in[pos++];// put it back in block k
        }
    }
}