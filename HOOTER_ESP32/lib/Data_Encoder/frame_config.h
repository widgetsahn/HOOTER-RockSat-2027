// FILE: \Data_Encoder\frame_config.h
// system include files
//

#ifndef FRAME_CONFIG_H
#define FRAME_CONFIG_H

#include "stdint.h" // we need fixed width integer types
#include "stddef.h" // we need size_t

// values that we will use throughout the encoder and decoder design to save on
// memory and processing time
// ==============================
//

// used to identify the start of the frame. The encoder writes
// it to each frame and the decoder looks for it to identify the start of a
// frame. we only need one at the start because the decoder will know how long
//the frame is and can use that to find the next frame
constexpr uint32_t SYNC_MARKER = 0x1ACFFC1D;// 4 bytes
constexpr size_t   SYNC_LEN = 4;   // bytes
// ==============================
// ====== calculations for REED-SOLOMON ======
// represents the total size of a block (DIFFERENT FROM FRAME!!!)
constexpr size_t   RS_N = 255; //bytes 

// represents how much of RS_N is actual data and not parity
constexpr size_t   RS_K = 223;   //bytes

// how many RS blocks we will shuffle together into each frame, blocks are 
// components of a frame
constexpr size_t   INTERLEAVE_DEPTH = 4; //blocks
//===== END OF REED-SOLOMON ======
//==============================

// size of the CRC-16 appended to the end of the frame data
constexpr size_t   CRC_LEN = 2;  // bytes

//==============================
//==== FRAME LENGTH CALCULATIONS ====
//
// calculation for frame length with data only, not taking into account either
// the sync marker or parity bytes (i.e. the actual data we can send down)
constexpr size_t   FRAME_DATA_LEN  = RS_K * INTERLEAVE_DEPTH;   //bytes

// space available for the header and sensor data, this is the region the CRC
//is computed over
constexpr size_t   PAYLOAD_LEN = FRAME_DATA_LEN - CRC_LEN;  // 890 bytes

// calculation for the frame length in bytes that we will be sending down
constexpr size_t   FRAME_LEN = SYNC_LEN + RS_N * INTERLEAVE_DEPTH; // 1024 bytes 
// ==== END OF FRAME LENGTH CALCULATIONS ====
//==============================

// === HEADER CONTENTS ===

// increments by 1 every frame; gaps on the ground show lost frames
constexpr size_t HDR_COUNT_LEN = 2;
constexpr size_t HDR_COUNT_OFF = 0;  // 0 bytes

// milliseconds since power-on when the frame was built
constexpr size_t HDR_TIME_LEN = 4;
constexpr size_t HDR_TIME_OFF = HDR_COUNT_OFF + HDR_COUNT_LEN; // 2bytes 

// which of the 4 antennas was active
constexpr size_t HDR_ANTENNA_LEN = 1;
constexpr size_t HDR_ANTENNA_OFF = HDR_TIME_OFF + HDR_TIME_LEN; // 6 bytes 

// how many bytes of real sensor data follow (0 = filler frame)
constexpr size_t HDR_DLEN_LEN = 2;
constexpr size_t HDR_DLEN_OFF = HDR_ANTENNA_OFF + HDR_ANTENNA_LEN;//7 bytes

// total header length summing the above 
constexpr size_t HEADER_LEN = HDR_DLEN_OFF + HDR_DLEN_LEN;  // 9 bytes
// === END OF HEADER CONTENTS ===
// ==============================

// =============================
// ==== FRAME DATA POSITIONS ====

// sensor data starts right after the header and fills the rest of the
// CRC-covered region
constexpr size_t SENSOR_DATA_OFF = HEADER_LEN;  // starts at frame[9]
constexpr size_t SENSOR_DATA_LEN = PAYLOAD_LEN - HEADER_LEN; // 881 bytes

// the CRC is the last 2 bytes of the frame data
constexpr size_t CRC_OFF = PAYLOAD_LEN;  // frame[890..891]
// as you can see, 1024
// end of include file
//
#endif