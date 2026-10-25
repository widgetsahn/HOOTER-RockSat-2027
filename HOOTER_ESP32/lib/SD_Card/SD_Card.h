#ifndef SD_CARD_H
#define SD_CARD_H

// SD_Card.h written by Mason Troy Ahner with the help of chatGPT. 10/25/2026.
// Alas, i am not the best programmer, I like circuits more, but i do know how i want things to be
// and chatGPT will help me and teach me along the way while interfacing with this absolutely nutty group of libraries
// that espressif provides for implementing their product into actual developments.
// i just want to say if you are reading this, i hope that my comments help. It took me a LONG time to understand how half of this stuff works
// and i probably haven't even touched cpp for like a year. but hey, maybe i will write another class implementation object that will use less
// chatGPT. idk yet, espressiff is crazy man idk why they write all their documentation like this. I just know is that when i get hired as an embedded
// engineer, i hope that there is a guy who knows programming the way i "know" circuits.

#include <stdint.h> //Used to declare fixed width interger types

#include <cstdio> //Used for standard C file operations

#include "sdkconfig.h" //esp-idf build configuration

#include "driver/gpio.h" //used to interface with ESP32 GPIOs

#include "driver/sdmmc_host.h" //Driver for ESP32 SDMMC hardware peripheral
#include "sdmmc_cmd.h"         //Used to implement SD card protocol functions

#include "esp_vfs_fat.h" //Used to connect FAT filesystem to ESP-IDF's virtual File System

// define the SD_Card class
class SD_Card
{
public:
    bool connect(); // initializes the SD card connection and the FAT filesystem, returns true if connection is sucessful

    void disconnect(); // closes and disconnects the SD card

    bool makeFile(const char *path); // creates a file at the supplied path. returns true if sucessful

    bool deleteFile(const char *path); // deletes a file at the supplied path. returns true if sucessful

    bool pathExists(const char *path); // returns true if the supplied file path exists

    bool writeFile(const char *path, const char *text); // write text to path (overrites all data). Returns true if sucessful

    bool appendFile(const char *path, const char *text); // write text to path (appends to end of data). Returns true if sucessful

    bool openFile(const char *path, bool append = false); // opens a file to begin streaming a buffer of binary data to it. (used primarily to store photos to the SD card)

    uint16_t writeChunk(const uint8_t *buffer, uint16_t buffer_size); // append data stream of binary data to SD card. (returns count of bytes written to the stream)

    void closeFile(); // close the opened file, this way we can add more files.

    uint64_t used_storage(); // returns the ammount of used storage in bytes

    bool checkCommunication(); //sends a fresh status command to the card. returns true if status is OK

private:
    bool _ready = false; // a status bit that confirms permission to proceed with SD card operations

    sdmmc_card_t *_card = nullptr; // card information for esp_vfs_fat_sdmmc_mount()
    // used to run card operations and mounting/unmounting

    std::FILE* _file = nullptr; //standard C file stream. This file is used for chunk/buffer streaming data when writing photos to the SD card


    //okay below are some hardware/firmware peripherials that must be set in order to implent ESP IDF's SDmmc API features

    static constexpr const char* MOUNT_POINT = "/sdcard"; //used to append to SD file system API
    //for example if you were to write to /data.csv, we will PREpend (not append) to the front of the filepath to be named
    //"/sdcard/data.csv"

    //ESP32 has two hardware interfaces, Slot 0 and Slot 1
    //slot 0 doesn't work for us because some of the GPIO pins reserved for slot 0
    //are also used for thee modules flash-memory interface. I don't wanna deal with that LMAO

    static constexpr int SDMMC_SLOT = SDMMC_HOST_SLOT_1; //yes please lets use slot 1 lmao

    //we can either use 1 bit or 4 bit per clock when writing to SD card, lets use 1 per clock, then change this to 4 and see if we can notice the difference
    //in write speed :P
    static constexpr int BUS_WIDTH = 1; //lets use one bit for now but in our implementation have the flexibility for 4 bits

    //below are the pins used for SLOT1 SDMMC hardware interfacing
    static constexpr gpio_num_t CMD_PIN = GPIO_NUM_15;
    static constexpr gpio_num_t CLK_PIN = GPIO_NUM_14;
    static constexpr gpio_num_t D0_PIN = GPIO_NUM_2;
    static constexpr gpio_num_t D1_PIN = GPIO_NUM_4;
    static constexpr gpio_num_t D2_PIN = GPIO_NUM_12;
    static constexpr gpio_num_t D3_PIN = GPIO_NUM_13;



};

#endif // SD_CARD_H