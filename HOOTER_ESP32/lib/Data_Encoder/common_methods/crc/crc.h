// FILE: \Data_Encoder\common_methods\crc\crc.h

// system include files
//

#ifndef CRC_H
#define CRC_H

#include "stdint.h" // we need fixed width integer types
#include "stddef.h" // we need size_t

// function that we will use to calculate CRC-16-CCITT, an error detection 
// algorithm standardized by CCITT and is a CCSDS standard. It essentially 
// treats the data as a polynomial and divides it by a generator polynomial,
// with it being a 16-bit polynomial with a value of 0x1021.,  if the
//recalculated CRC does not match the received CRC, the data is corrupted
// ==============================
//
uint16_t crc16_ccitt(const uint8_t* data, size_t len);

// end of include file
//
#endif