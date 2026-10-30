//neopixel.h written by chatGPT to turn off the neopixel LED on the development board

#ifndef NEO_PIXEL_H
#define NEO_PIXEL_H

#include <cstdint>
#include "driver/gpio.h"
#include "driver/rmt_tx.h"
#include "driver/rmt_encoder.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

// One WS2812-compatible RGB pixel, GRB order, ESP-IDF 5.x or newer.
// Call begin() once before sharing with tasks. Calls to setColor()/off()
// are mutex-protected and synchronous; do not call them from an ISR.
// Stop all callers before destroying this object.
class NeoPixel
{
public:
    explicit NeoPixel(gpio_num_t pin);
    ~NeoPixel();

    NeoPixel(const NeoPixel&) = delete;
    NeoPixel& operator=(const NeoPixel&) = delete;
    NeoPixel(NeoPixel&&) = delete;
    NeoPixel& operator=(NeoPixel&&) = delete;

    esp_err_t begin(); // Allocate resources and transmit black.
    esp_err_t setColor(uint8_t red, uint8_t green, uint8_t blue);
    esp_err_t off();

private:
    esp_err_t transmitColor(uint8_t red, uint8_t green, uint8_t blue);
    void releaseResources();

    gpio_num_t _pin;
    rmt_channel_handle_t _channel = nullptr;
    rmt_encoder_handle_t _encoder = nullptr;
    SemaphoreHandle_t _mutex = nullptr;
    rmt_symbol_word_t _frame[26] = {};
    bool _enabled = false;
    bool _ready = false;
};

#endif // NEO_PIXEL_H
