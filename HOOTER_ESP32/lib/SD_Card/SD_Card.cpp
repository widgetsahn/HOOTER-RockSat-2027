// SD_Card.cpp

// written by Mason Ahner 10/25/2026

#include <cstring> //used for stringlength

#include "SD_Card.h" //header file for class definition

// little helper function to check if its a valid file path
static bool validPath(const char *path)
{
    // check to see if the paths pointer isnt null
    // check to see if the first character of the file path has a '/'
    // check to see if the second character isnt blank
    return (path != nullptr) && (path[0] == '/') && (path[1] != '\0');
}

// Initialize SDMMC and connect to SD card host
// TODO:
bool SD_Card::connect()
{
    // first, whenever we connect, we should disconnect the SD_Card and then try to re-connect
    disconnect();

    // check to make sure _file and _card resources are inactive
    if (_card != nullptr || _file != nullptr)
    {
        return false;
    }

    // configure the ESP32's SDMMC controller
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();

    // set the host to slot 1 and set the operating clock to 20MHz
    host.slot = SDMMC_SLOT;
    host.max_freq_khz = SDMMC_FREQ_DEFAULT; // 20,000kHz (20MHz)

    // configure the MMC host using the default hardware pin assignments
    sdmmc_slot_config_t slot = SDMMC_SLOT_CONFIG_DEFAULT();

    // set the bus width of the slot use either 1 or 4 depending on .h implementation
    slot.width = BUS_WIDTH;

    // set the mount configuration with FAT filesystem
    esp_vfs_fat_mount_config_t mount_config = {};
    mount_config.format_if_mount_failed = false; // we need to make sure if the mount failed we don't wipe our data lol
    mount_config.max_files = MAX_FILES;          // have up to MAX_FILES max_files

    // initlize controller and card then mount the filesystem

    const esp_err_t mount_result = esp_vfs_fat_sdmmc_mount(
        MOUNT_POINT,   // the mounting point (default)
        &host,         // pointer to the host (default)
        &slot,         // pointer to the slot (slot 1, width of 1)
        &mount_config, // MAX_FILES number of maximum open files, don't format if connection failed
        &_card);

    // upon attempting this connection, we can check mount_result to see if it was sucessful
    if (mount_result != ESP_OK) // if its not ok, then we screwed up cuh
    {
        return false;
    }

    const esp_err_t status_result = sdmmc_get_status(_card); // run an additional check to make sure we are in good status
    if (status_result != ESP_OK)
    {
        disconnect();
        return false;
    }

    // if we passed the redundant check, we can finally declare that we are ready to rumble!
    _ready = true;
    return true;
}

// TODO:
void SD_Card::disconnect()
{
}

// TODO:
bool SD_Card::makeFile(const char *path)
{
    return false;
}

// TODO:
bool SD_Card::deleteFile(const char *path)
{
    return false;
}

// TODO:
bool SD_Card::pathExists(const char *path)
{
    return false;
}

// TODO:
bool SD_Card::writeFile(const char *path, const char *text)
{
    return false;
}

// TODO:
bool SD_Card::appendFile(const char *path, const char *text)
{
    return false;
}

// TODO:
bool SD_Card::openFile(const char *path, bool append)
{
    return false;
}

// TODO:
uint16_t SD_Card::writeChunk(const uint8_t *buffer, uint16_t buffer_size)
{
    return 0;
}

// TODO:
void SD_Card::closeFile()
{
}

// TODO:
uint64_t SD_Card::used_storage()
{
    return 0;
}

// TODO:
bool SD_Card::checkCommunication()
{
    // first we know that if the SD card hasn't been initialized, we can already just return false
    if (!_ready)
    {
        return false;
    }
    _ready = false;
    return _ready;
}
