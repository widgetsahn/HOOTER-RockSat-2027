// FILE: \Data_Encoder\common_methods\interleaver\interleaver.h

// system include files
//

#ifndef INTERLEAVER_H
#define INTERLEAVER_H

#include "stdint.h" // we need fixed width integer types

/* below are functions that we will use to "interleave" and "deinterleave". 
Interleaving is a commonly used method to weave together data, and we're
 doing it here for the sake of reducing the chance of there being a big
 chunk of data that is lost due to a single bit error. Because we're planning
 on transmitting approximately 1020 bytes of data at a time between 4 blocks
 of data, the whole idea is to weave this data together so that if a single 
 bit error occurs, it will only affect a small portion of each block, rather
 than a large portion of one block. 

 for Example: 
 if we have 4 blocks (A-D) of data, each with 255 bytes, without
 interleaving, this is how it will be sent: 

 A0, A1, A2 ... A254, B0, B1, B2 ... etc. 

 This is problematic, because if we have a dropout that causes, say a 10 byte
 chunk of data to disappear, we will lose a large portion of a singular block,
 which is bad for Reed-Solomon error correction.

 However, if we interleave the data, it will be sent like this:
 A0, B0, C0, D0, A1, B1, C1, D1 ... etc.

 By doing this with our data, if we use that same dropout example from up
 above, we will only lose a small portion of every block, which is much better
 for Reed-Solomon error correction.

*/
// ==============================
// to be used by the ESP32
//
void interleave(const uint8_t* in, uint8_t* out);
// to be used for ground station
//
void deinterleave(const uint8_t* in, uint8_t* out); 

// end of include file
//
#endif