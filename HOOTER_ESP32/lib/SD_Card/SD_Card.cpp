// SD_Card.cpp

// written by Mason Ahner 10/25/2026

#include <cstring>    //used for stringlength
#include <string>     //used to prepend /sdcard to the filepath
#include "SD_Card.h"  //header file for class definition
#include <sys/stat.h> // used to look at the status of a file and run file-type checks
#include <cerrno>     //used to help distinguish between a missing path and a filesystem error

// little helper function to check if its a valid file path
static bool validPath(const char *path)
{
    // check to see if the paths pointer isnt null
    // check to see if the first character of the file path has a '/'
    // check to see if the second character isnt blank
    return (path != nullptr) && (path[0] == '/') && (path[1] != '\0');
}

// Initialize SDMMC and connect to SD card host
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

void SD_Card::disconnect()
{
    // prevent SD card operations
    _ready = false;

    // check to see if a file is open for chunk streaming
    if (_file != nullptr)
    {
        const int close_result = std::fclose(_file); // try to close file and record its result

        // check to see if the close was sucessful
        if (close_result != 0)
        {
            printf("SD_Card.disconnect(): Failed fclose\n");
        }

        // regardless if the close was sucessful, we need to set _file to nullptr
        _file = nullptr;
    }
    // now lets check to see if a card has been mounted
    if (_card != nullptr)
    {
        const esp_err_t unmount_result = esp_vfs_fat_sdcard_unmount(MOUNT_POINT, _card); // run a dismount of the SD card

        // check to see if the unmount was sucessful
        if (unmount_result != ESP_OK)
        {
            // if the unmount was unsucessful, we need to end here
            printf("SD_Card.disconnect(): Failed esp_vfs_fat_sd_unmount\n");
            return;
        }

        // if the unmount suceeded, the card pointer is no longer needed
        _card = nullptr;
    }
}

bool SD_Card::makeFile(const char *path)
{
    // before we try making a file, lets check the following
    // 1. make sure that the SD card is ready
    // 2. make sure that a chunk file is not open
    // 3. make sure that the specified file path is a valid path name
    if (!_ready || _file || !validPath(path))
    {
        return false;
    }

    // handle cases where the file path already exists
    if (pathExists(path) || !_ready) // we could encounter a connection issue during pathexists
    {
        return false;
    }

    // prepend /sdcard to file path
    const std::string prepended_path = std::string(MOUNT_POINT) + path;

    // create empty file
    std::FILE *file = std::fopen(prepended_path.c_str(), "wb");

    // check to see if the file was sucessfully created
    if (file == nullptr)
    {
        _ready = false;
        return false;
    }

    // attempt to close the file and check if the file was sucessfully closed
    const int close_result = std::fclose(file);

    if (close_result != 0) // if the close result returned a non-zero value, the close attempt failed
    {
        _ready = false;
        return false;
    }

    // up to this point, if we haven't failed any tests, its safe to say we managed to sucessfully created a file on the SD card
    return true;
}

bool SD_Card::deleteFile(const char *path)
{
    // check the following
    // 1. Make sure the SD card is ready
    // 2. Make sure we aren't chunk streaming
    // 3. Make sure the provided file path is a valid path
    if (!_ready || _file || !validPath(path))
    {
        return false;
    }

    // check to see if the path exists or if the existence check failed
    if (!pathExists(path) || !_ready)
    {
        return false;
    }

    // prepend the filesystem mount point to the file path
    const std::string prepend_path = std::string(MOUNT_POINT) + path;

    // get the info of this file path
    struct stat file_info = {};

    // check to see if we obtained the file info
    if (stat(prepend_path.c_str(), &file_info))
    {
        _ready = false;
        return false;
    }

    // check to see if the file is a directory
    if (!S_ISREG(file_info.st_mode))
    {
        printf("SD_Card.deleteFile(const char *path): Path is not a regular file!\n");
        return false;
    }

    // Okay if the file isn't a directory, lets try and delete it!
    if (std::remove(prepend_path.c_str()) != 0)
    {
        // if file deletion failed we screwed up
        _ready = false;
        return false;
    }

    // if we passed all of our tests, lets return true :D
    return true;
}

bool SD_Card::pathExists(const char *path)
{
    // first, lets see if we are ready and that its a valid path.
    if (!_ready || !validPath(path))
    {
        return false;
    }

    // prepend "/sdcard" to the supplied path
    const std::string prepended_path = std::string(MOUNT_POINT) + path;

    // get the information about the path
    struct stat file_info = {};

    // check to see if a file or directory exists at this path
    if (stat(prepended_path.c_str(), &file_info) == 0)
    {
        // if a file/directory exists at this path, we know that the path exists!
        return true;
    }

    // if a file/directory doesnt exist, check to the error indicator to see its because we explicitly looked for the path and couldn't find it
    if (errno == ENOENT)
    {
        return false;
    }

    // any other possible errors would indicate a communication or filesystem failure
    _ready = false;
    return false;
}

// TODO:
bool SD_Card::writeFile(const char *path, const char *text)
{
    // before we try anything we must validate the following
    // 1. the SD card is ready to write
    // 2. we aren't chunk writing to a file
    // 3. the path provided exists
    // 4. the text isnt null lol
    if (_file || !pathExists(path) || text == nullptr || !_ready)
    {
        return false;
    }

    //prepend the file
    const std::string prepended_path = std::string(MOUNT_POINT) + path;

    //get info about the file path
    struct stat file_info = {};

    //if we could not retrieve statistics about the file path, set _ready = false and return false
    if (stat(prepended_path.c_str(), &file_info))
    {
        _ready = false;
        return false;
    }

    //check to see if the filepath was refering to a regular file (if not return false)
    if (!S_ISREG(file_info.st_mode))
    {
        return false;
    }

    //if the file exists, and we could confirm that the file path is for a regular file, we can attempt to write

    //open the file for writing
    std::FILE *file = std::fopen(prepended_path.c_str(), "wb");

    //check to see if file open was sucessful
    if (file == nullptr)
    {
        _ready = false;
        return false;
    }

    //lets look at the ammount of bytes of text that will be written to the SD card
    const size_t length = std::strlen(text);

    //write to the file and get the number of bytes written
    const size_t written = std::fwrite(text, 1, length, file);

    //attempt to close the file
    const int close_result = std::fclose(file);

    //check to see if close was sucessful
    if (close_result)
    {
        _ready = false;
        printf("SD_Card.writeFile(): File close attempt fail!");
        return false;
    }

    //check to see if the write was sucessful

    if (written != length)
    {
        _ready = false;
        printf("SD_Card.writeFile(): File bytes written != # of bytes sent to writeFile");
        return false;
    }

    //if we sucessfully wrote the correct amount of bytes to the file AND we sucessfully closed it,
    //we can concur that our writeFile() was sucessful!

    return true;
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
