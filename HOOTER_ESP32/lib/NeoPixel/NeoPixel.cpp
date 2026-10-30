#include "NeoPixel.h"

namespace
{
// Common WS2812 timing profile; confirm against your actual LED datasheet.
// At 40 MHz, each tick lasts 25 ns.
constexpr uint32_t RMT_RESOLUTION_HZ = 40000000;
constexpr uint16_t ZERO_HIGH_TICKS = 14; // 350 ns
constexpr uint16_t ZERO_LOW_TICKS  = 36; // 900 ns
constexpr uint16_t ONE_HIGH_TICKS  = 36; // 900 ns
constexpr uint16_t ONE_LOW_TICKS   = 14; // 350 ns
constexpr uint16_t RESET_HALF_TICKS = 8000; // Two halves give 400 us LOW.
}

NeoPixel::NeoPixel(gpio_num_t pin) : _pin(pin) {}

NeoPixel::~NeoPixel()
{
    // Destruction must occur only after all tasks stop using this object.
    releaseResources();
}

void NeoPixel::releaseResources()
{
    _ready = false;
    if (_enabled)
    {
        (void)rmt_disable(_channel);
        _enabled = false;
    }
    if (_encoder != nullptr)
    {
        (void)rmt_del_encoder(_encoder);
        _encoder = nullptr;
    }
    if (_channel != nullptr)
    {
        (void)rmt_del_channel(_channel);
        _channel = nullptr;
    }
    if (_mutex != nullptr)
    {
        vSemaphoreDelete(_mutex);
        _mutex = nullptr;
    }
}

esp_err_t NeoPixel::begin()
{
    // Initialization is deliberately separate from the constructor:
    // global objects are constructed before app_main() starts.
    if (_mutex != nullptr)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (!GPIO_IS_VALID_OUTPUT_GPIO(_pin))
    {
        return ESP_ERR_INVALID_ARG;
    }

    _mutex = xSemaphoreCreateMutex();
    if (_mutex == nullptr)
    {
        return ESP_ERR_NO_MEM;
    }

    rmt_tx_channel_config_t channelConfig = {};
    channelConfig.gpio_num = _pin;
    channelConfig.clk_src = RMT_CLK_SRC_DEFAULT;
    channelConfig.resolution_hz = RMT_RESOLUTION_HZ;
    channelConfig.mem_block_symbols = 64;
    channelConfig.trans_queue_depth = 1;

    esp_err_t result = rmt_new_tx_channel(&channelConfig, &_channel);
    if (result != ESP_OK)
    {
        releaseResources();
        return result;
    }

    rmt_copy_encoder_config_t encoderConfig = {};
    result = rmt_new_copy_encoder(&encoderConfig, &_encoder);
    if (result != ESP_OK)
    {
        releaseResources();
        return result;
    }

    result = rmt_enable(_channel);
    if (result != ESP_OK)
    {
        releaseResources();
        return result;
    }
    _enabled = true;

    // Send black before begin() reports success.
    result = transmitColor(0, 0, 0);
    if (result != ESP_OK)
    {
        releaseResources();
        return result;
    }
    _ready = true;
    return ESP_OK;
}

esp_err_t NeoPixel::setColor(uint8_t red, uint8_t green, uint8_t blue)
{
    if (_mutex == nullptr)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (xSemaphoreTake(_mutex, portMAX_DELAY) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }

    esp_err_t result = ESP_ERR_INVALID_STATE;
    if (_ready)
    {
        result = transmitColor(red, green, blue);
        if (result != ESP_OK)
        {
            // Prevent further writes after a driver failure.
            _ready = false;
        }
    }

    xSemaphoreGive(_mutex);
    return result;
}

esp_err_t NeoPixel::off()
{
    return setColor(0, 0, 0);
}

esp_err_t NeoPixel::transmitColor(uint8_t red, uint8_t green, uint8_t blue)
{
    // Initial LOW resets the receiver; final LOW latches the new color.
    _frame[0].level0 = 0;
    _frame[0].duration0 = RESET_HALF_TICKS;
    _frame[0].level1 = 0;
    _frame[0].duration1 = RESET_HALF_TICKS;
    _frame[25] = _frame[0];

    const uint8_t bytes[3] = {green, red, blue};
    unsigned index = 1;
    for (uint8_t byte : bytes)
    {
        for (int bit = 7; bit >= 0; --bit)
        {
            const bool one = (byte & (1U << bit)) != 0;
            rmt_symbol_word_t &symbol = _frame[index++];
            symbol.level0 = 1;
            symbol.duration0 = one ? ONE_HIGH_TICKS : ZERO_HIGH_TICKS;
            symbol.level1 = 0;
            symbol.duration1 = one ? ONE_LOW_TICKS : ZERO_LOW_TICKS;
        }
    }

    rmt_transmit_config_t config = {};
    config.loop_count = 0;
    config.flags.eot_level = 0;

    esp_err_t result = rmt_transmit(
        _channel, _encoder, _frame, sizeof(_frame), &config
    );
    if (result != ESP_OK)
    {
        return result;
    }

    // Do not release the mutex or modify the frame until TX finishes.
    return rmt_tx_wait_all_done(_channel, -1);
}
