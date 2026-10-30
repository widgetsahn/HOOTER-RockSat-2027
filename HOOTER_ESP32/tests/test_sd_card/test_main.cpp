// SD Card Tests Written by chatGPT
// ESP-IDF / PlatformIO on-device SD_Card integration tests.
// Put this file in tests/test_sd_card/test_main.cpp.
// SD_Card.cpp stays in src/ and SD_Card.h in include/.
// Runs once per reset in one dedicated FreeRTOS task.
// Live serial progress uses printf(); no Arduino Serial dependency.
// Reports: /sdcard/SD_Card_tests/runNNNN/results.csv
// Existing runs are preserved. Only fixtures inside the new run are modified.
// GPIO25: optional external LED, active HIGH. ON=all passed and report saved.
// Unity serial output is authoritative for individual results.
// Raw stdio readback bypasses the wrapper, but uses the same FAT/SDMMC stack.
#include "SD_Card.h"
#include <unity.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <cerrno>
#include <sys/stat.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"

// Live results are printed immediately; Unity assertions follow cleanup.

namespace
{
    constexpr gpio_num_t LED = GPIO_NUM_25;
    constexpr const char *ROOT = "/sdcard";
    SD_Card card; // Accessed only by the SD test task.
    struct Result
    {
        const char *name;
        bool pass;
    };
    Result results[128];
    size_t count = 0;
    bool overflow = false;
    std::string run, text, binary, missing, directory;
    int64_t write_us = 0;

    void check(const char *name, bool pass)
    {
        std::printf("[SD TEST %03u] %s: %s\n",
                    static_cast<unsigned>(count + 1), pass ? "PASS" : "FAIL", name);
        std::fflush(stdout);
        if (count < sizeof(results) / sizeof(results[0]))
            results[count++] = {name, pass};
        else
            overflow = true;
    }
    std::string raw(const std::string &path) { return std::string(ROOT) + path; }
    bool equals(const std::string &path, const uint8_t *expected, size_t size)
    {
        std::FILE *f = std::fopen(raw(path).c_str(), "rb");
        if (!f)
            return false;
        uint8_t block[256];
        size_t offset = 0;
        bool ok = true;
        while (ok && offset < size)
        {
            size_t n = size - offset;
            if (n > sizeof(block))
                n = sizeof(block);
            if (std::fread(block, 1, n, f) != n || std::memcmp(block, expected + offset, n))
                ok = false;
            offset += n;
        }
        if (ok && (std::fgetc(f) != EOF || std::ferror(f)))
            ok = false;
        if (std::fclose(f))
            ok = false;
        return ok;
    }
    bool textEquals(const char *expected)
    {
        return equals(text, reinterpret_cast<const uint8_t *>(expected), std::strlen(expected));
    }
    bool prepareRun()
    {
        const std::string base = std::string(ROOT) + "/SD_Card_tests";
        if (mkdir(base.c_str(), 0777) != 0)
        {
            if (errno != EEXIST)
                return false;
            struct stat s = {};
            if (stat(base.c_str(), &s) || !S_ISDIR(s.st_mode))
                return false;
        }
        for (unsigned i = 1; i <= 9999; ++i)
        {
            char name[64];
            std::snprintf(name, sizeof(name), "/SD_Card_tests/run%04u", i);
            if (mkdir(raw(name).c_str(), 0777) == 0)
            {
                run = name;
                text = run + "/text.txt";
                binary = run + "/binary.bin";
                missing = run + "/missing.txt";
                directory = run + "/directory";
                return true;
            }
            if (errno != EEXIST)
                return false;
        }
        return false;
    }
    bool saveReport(bool complete)
    {
        std::FILE *f = std::fopen(raw(run + "/results.csv").c_str(), "wb");
        if (!f)
            return false;
        bool ok = std::fprintf(f, "test,result\n") >= 0;
        unsigned passed = 0;
        for (size_t i = 0; i < count; ++i)
        {
            if (std::fprintf(f, "%s,%s\n", results[i].name, results[i].pass ? "PASS" : "FAIL") < 0)
                ok = false;
            if (results[i].pass)
                ++passed;
        }
        if (std::fprintf(f, "passed,%u\nfailed,%u\noverflow,%s\n64KiB_write_and_close_us,%lld\nstatus,%s\n",
                         passed, static_cast<unsigned>(count) - passed, overflow ? "YES" : "NO",
                         static_cast<long long>(write_us), complete ? "COMPLETE" : "RUNNING") < 0)
            ok = false;
        if (std::fclose(f))
            ok = false;
        return ok;
    }
    void runTests()
    {
        check("communication after connect", card.checkCommunication());
        check("missing path absent", !card.pathExists(missing.c_str()));
        check("create text", card.makeFile(text.c_str()));
        check("created path exists", card.pathExists(text.c_str()));
        check("new file empty", textEquals(""));
        check("write initial text", card.writeFile(text.c_str(), "Original long contents\n"));
        check("initial text readback", textEquals("Original long contents\n"));
        check("reject existing file creation", !card.makeFile(text.c_str()));
        check("creation preserves existing content", textEquals("Original long contents\n"));
        check("overwrite shorter", card.writeFile(text.c_str(), "Hi"));
        check("overwrite truncates", textEquals("Hi"));
        check("append text", card.appendFile(text.c_str(), " there\n"));
        check("append readback", textEquals("Hi there\n"));
        check("append empty", card.appendFile(text.c_str(), ""));
        check("empty append preserves content", textEquals("Hi there\n"));
        check("write empty", card.writeFile(text.c_str(), ""));
        check("empty write truncates", textEquals(""));
        check("reject null path", !card.makeFile(nullptr));
        check("reject empty path", !card.makeFile(""));
        check("reject relative path", !card.makeFile("relative.txt"));
        check("reject root path", !card.makeFile("/"));
        check("reject null text", !card.writeFile(text.c_str(), nullptr));
        check("reject null append", !card.appendFile(text.c_str(), nullptr));
        check("reject missing write", !card.writeFile(missing.c_str(), "X"));
        check("reject missing append", !card.appendFile(missing.c_str(), "X"));
        check("reject missing open", !card.openFile(missing.c_str()));
        check("reject missing delete", !card.deleteFile(missing.c_str()));
        // A real operation proves the wrapper still permits writes.
        check("expected rejections preserve usability", card.writeFile(text.c_str(), "OK"));
        bool made = mkdir(raw(directory).c_str(), 0777) == 0;
        check("create directory fixture", made);
        if (made)
        {
            check("directory exists", card.pathExists(directory.c_str()));
            check("reject directory creation", !card.makeFile(directory.c_str()));
            check("reject directory write", !card.writeFile(directory.c_str(), "X"));
            check("reject directory append", !card.appendFile(directory.c_str(), "X"));
            check("reject directory open", !card.openFile(directory.c_str()));
            check("reject directory deletion", !card.deleteFile(directory.c_str()));
            check("directory rejection preserves usability", card.writeFile(text.c_str(), "OK"));
        }
        const uint8_t a[] = {0, 1, 127, 128, 255}, b[] = {66, 0, 77};
        const uint8_t both[] = {0, 1, 127, 128, 255, 66, 0, 77};
        check("create binary", card.makeFile(binary.c_str()));
        check("reject chunk without open", card.writeChunk(a, sizeof(a)) == 0);
        bool opened = card.openFile(binary.c_str());
        check("open binary overwrite", opened);
        if (opened)
        {
            check("reject second open", !card.openFile(text.c_str()));
            check("reject delete while streaming", !card.deleteFile(text.c_str()));
            check("reject text write while streaming", !card.writeFile(text.c_str(), "X"));
            check("reject append while streaming", !card.appendFile(text.c_str(), "X"));
            check("reject creation while streaming", !card.makeFile(missing.c_str()));
            check("path check while streaming", card.pathExists(text.c_str()));
            check("reject null buffer", card.writeChunk(nullptr, 5) == 0);
            check("reject zero size", card.writeChunk(a, 0) == 0);
            check("first chunk count", card.writeChunk(a, sizeof(a)) == sizeof(a));
            check("second chunk count", card.writeChunk(b, sizeof(b)) == sizeof(b));
        }
        card.closeFile();
        check("binary readback", equals(binary, both, sizeof(both)));
        card.closeFile();
        check("close twice preserves usability", card.writeFile(text.c_str(), "OK"));
        check("reject chunk after close", card.writeChunk(a, sizeof(a)) == 0);
        opened = card.openFile(binary.c_str(), true);
        check("open binary append", opened);
        if (opened)
            check("append chunk count", card.writeChunk(a, sizeof(a)) == sizeof(a));
        card.closeFile();
        const uint8_t appended[] = {0, 1, 127, 128, 255, 66, 0, 77, 0, 1, 127, 128, 255};
        check("binary append readback", equals(binary, appended, sizeof(appended)));
        opened = card.openFile(binary.c_str(), false);
        check("reopen binary overwrite", opened);
        if (opened)
            check("overwrite chunk count", card.writeChunk(b, sizeof(b)) == sizeof(b));
        card.closeFile();
        check("binary overwrite truncates", equals(binary, b, sizeof(b)));
        static uint8_t buffer[4096];
        for (size_t i = 0; i < sizeof(buffer); ++i)
            buffer[i] = static_cast<uint8_t>(i);
        opened = card.openFile(binary.c_str(), false);
        check("open multi sector write", opened);
        bool full = opened;
        const int64_t start = esp_timer_get_time();
        if (opened)
            for (unsigned i = 0; i < 16; ++i)
            {
                if (card.writeChunk(buffer, sizeof(buffer)) != sizeof(buffer))
                {
                    full = false;
                    break;
                }
            }
        card.closeFile();
        write_us = esp_timer_get_time() - start; // Includes final close/flush, excludes open.
        check("64KiB write counts", full);
        std::FILE *f = std::fopen(raw(binary).c_str(), "rb");
        bool verified = f != nullptr;
        if (f)
        {
            uint8_t readback[512];
            for (unsigned n = 0; verified && n < 128; ++n)
            {
                if (std::fread(readback, 1, sizeof(readback), f) != sizeof(readback))
                {
                    verified = false;
                    break;
                }
                for (size_t i = 0; i < sizeof(readback); ++i)
                    if (readback[i] != static_cast<uint8_t>(i))
                    {
                        verified = false;
                        break;
                    }
            }
            if (verified && (std::fgetc(f) != EOF || std::ferror(f)))
                verified = false;
            if (std::fclose(f))
                verified = false;
        }
        check("64KiB byte for byte readback", verified);
        std::printf("64 KiB write plus close: %lld us; write counts=%s; readback=%s\n",
                    static_cast<long long>(write_us), full ? "OK" : "FAILED",
                    verified ? "OK" : "FAILED");
        if (full && verified && write_us > 0) {
            std::printf("Measured rate: %.3f MB/s (small functional test)\n",
                        65536.0 / static_cast<double>(write_us));
        }
        uint64_t total = 0, free = 0;
        const esp_err_t info = esp_vfs_fat_info(ROOT, &total, &free);
        const uint64_t used = card.used_storage();
        check("used space agrees with filesystem", info == ESP_OK && used == total - free && used >= 65536);
        check("delete text", card.deleteFile(text.c_str()));
        struct stat s = {};
        const int status = stat(raw(text).c_str(), &s);
        check("deleted text absent raw", status != 0 && errno == ENOENT);
        check("deleted text absent wrapper", !card.pathExists(text.c_str()));
        // Exercise disconnect while a stream is open, and verify close flushed it.
        opened = card.openFile(binary.c_str(), false);
        check("open before disconnect", opened);
        if (opened)
            check("write before disconnect", card.writeChunk(a, sizeof(a)) == sizeof(a));
        card.disconnect();
        card.disconnect();
        check("disconnected communication rejected", !card.checkCommunication());
        check("disconnected path check rejected", !card.pathExists(binary.c_str()));
        check("disconnected creation rejected", !card.makeFile(text.c_str()));
        check("disconnected storage zero", card.used_storage() == 0);
        check("disconnected chunk rejected", card.writeChunk(a, sizeof(a)) == 0);
        check("reconnect", card.connect());
        check("communication after reconnect", card.checkCommunication());
        check("disconnect flushed open stream", equals(binary, a, sizeof(a)));
        check("reconnect while mounted", card.connect());
        check("persistent path after reconnect", card.pathExists(binary.c_str()));
    }
    // Unity assertions run after cleanup so a failed assertion cannot leave files open.
    size_t current_result = 0;
    void verifyRecordedResult()
    {
        TEST_ASSERT_TRUE_MESSAGE(results[current_result].pass, results[current_result].name);
    }
    void verifySetup()
    {
        TEST_ASSERT_TRUE_MESSAGE(false, "Could not connect or create the SD report directory");
    }
    void verifyReport()
    {
        TEST_ASSERT_TRUE_MESSAGE(false, "Could not finish saving results.csv");
    }
    void verifyOverflow()
    {
        TEST_ASSERT_TRUE_MESSAGE(!overflow, "Result buffer overflow");
    }
    void sdTask(void *)
    {
        gpio_reset_pin(LED);
        gpio_set_direction(LED, GPIO_MODE_OUTPUT);
        gpio_set_level(LED, 0);
        UNITY_BEGIN();
        std::printf("\n=== SD card tests starting ===\n");
        std::printf("Connecting to SD card...\n");
        std::fflush(stdout);
        const bool initial = card.connect();
        check("initial connect", initial);
        bool connected = initial;
        if (!initial)
        {
            connected = card.connect();
            check("initial connect retry", connected);
        }
        bool prepared = false;
        if (connected) {
            std::printf("Creating a new test directory...\n");
            if (prepareRun()) {
                std::printf("Report: %s/results.csv\n", raw(run).c_str());
                prepared = saveReport(false);
                if (!prepared) std::printf("ERROR: Could not save initial CSV report.\n");
            } else {
                std::printf("ERROR: Could not create test directory (errno=%d).\n", errno);
            }
        } else {
            std::printf("ERROR: Card connection failed twice; tests cannot proceed.\n");
        }
        std::fflush(stdout);
        bool complete = false;
        if (prepared)
        {
            runTests();
            // A communication success does not reset a latched readiness fault.
            const bool reportConnected = card.connect();
            check("final report reconnect", reportConnected);
            if (reportConnected)
                complete = saveReport(true);
        }
        card.disconnect();
        bool failed = overflow || !prepared || !complete;
        for (size_t i = 0; i < count; ++i)
            if (!results[i].pass)
                failed = true;
        unsigned passed = 0;
        for (size_t i = 0; i < count; ++i) if (results[i].pass) ++passed;
        std::printf("\n=== SD checks: %u passed / %u failed ===\n",
                    passed, static_cast<unsigned>(count) - passed);
        std::printf("CSV report: %s; overall: %s\n",
                    complete ? "SAVED" : "NOT COMPLETED", failed ? "FAIL" : "PASS");
        std::fflush(stdout);
        // Steady HIGH indicates the report completed and all recorded tests passed.
        gpio_set_level(LED, complete && !failed ? 1 : 0);
        if (!prepared)
            RUN_TEST(verifySetup);
        for (current_result = 0; current_result < count; ++current_result)
        {
            // Use each recorded condition as its Unity test name.
            UnityDefaultTestRun(verifyRecordedResult, results[current_result].name, __LINE__);
        }
        RUN_TEST(verifyOverflow);
        if (prepared && !complete)
            RUN_TEST(verifyReport);
        UNITY_END();
        std::fflush(stdout);
        vTaskDelete(nullptr);
    }
}
void setUp() {}
void tearDown() {}
extern "C" void app_main()
{
    // ESP-IDF stack size is bytes. Only this task accesses the SD card.
    if (xTaskCreate(sdTask, "sd_tests", 12288, nullptr, 5, nullptr) != pdPASS)
    {
        UNITY_BEGIN();
        RUN_TEST(verifySetup);
        UNITY_END();
    }
}
