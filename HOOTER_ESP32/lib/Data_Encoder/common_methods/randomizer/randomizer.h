// FILE: \Data_Encoder\common_methods\randomizer\randomizer.h

// system include files
//

#ifndef RANDOMIZER_H
#define RANDOMIZER_H

#include "stdint.h" // we need fixed width integer types
#include "stddef.h" // we need size_t

// ==============================
// function that we will use to randomize the frame of data that we plan on
// transmitting out
//
void randomize(uint8_t* data, size_t len);
// for reciever on the ground 
//
void derandomize(uint8_t* data, size_t len);

// end of include file
//
#endif