#pragma once
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <cstring>
#include <algorithm>

class SSD1680
{
    inline static constexpr const char *TAG = "SSD1680";
public:
    static constexpr uint16_t WIDTH  = 122;
    static constexpr uint16_t HEIGHT = 250;
    static constexpr uint16_t BYTES_PER_LINE = (WIDTH + 7) / 8;
    static constexpr uint32_t FRAME_BYTES = (uint32_t)BYTES_PER_LINE * HEIGHT;

    SSD1680() = default;

    esp_err_t Init(spi_device_handle_t spi, gpio_num_t pinDc, gpio_num_t pinRst, gpio_num_t pinBusy);
    void WriteFrame(const uint8_t* frameData);
    void Update();
    void Sleep();
    void WakeUp();

private:
    spi_device_handle_t _spi = nullptr;
    gpio_num_t _pinDc = GPIO_NUM_NC;
    gpio_num_t _pinRst = GPIO_NUM_NC;
    gpio_num_t _pinBusy = GPIO_NUM_NC;

    static constexpr bool BUSY_ACTIVE_LOW = true;

    bool IsBusy() const;
    bool WaitBusy(uint32_t timeoutMs = 5000) const;

    void WriteCmd(uint8_t cmd) const;
    void WriteData(const uint8_t* data, size_t len) const;
};
