#pragma once
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"

class EpaperDriver {
    inline static constexpr const char *TAG = "EpaperDriver";
public:
    EpaperDriver();  // default ctor

    esp_err_t init(spi_device_handle_t spi,
                   gpio_num_t pinBusy,
                   gpio_num_t pinReset,
                   gpio_num_t pinDc);

    void reset() const;
    void wait_ready() const;

    void send_command(uint8_t cmd) const;
    void send_data(const uint8_t* data, size_t len) const;

    void send_framebuffer(const uint8_t* buffer, size_t len) const;

private:
    spi_device_handle_t spi_ = nullptr;
    gpio_num_t pinBusy_ = GPIO_NUM_NC;
    gpio_num_t pinReset_ = GPIO_NUM_NC;
    gpio_num_t pinDc_ = GPIO_NUM_NC;
};
