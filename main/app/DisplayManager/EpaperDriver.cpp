#include "EpaperDriver.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"



EpaperDriver::EpaperDriver() {}

esp_err_t EpaperDriver::init(spi_device_handle_t spi,
                             gpio_num_t pinBusy,
                             gpio_num_t pinReset,
                             gpio_num_t pinDc) {
    spi_ = spi;
    pinBusy_ = pinBusy;
    pinReset_ = pinReset;
    pinDc_ = pinDc;

    // Configure GPIOs
    gpio_config_t io = {};
    io.mode = GPIO_MODE_OUTPUT;
    io.pin_bit_mask = (1ULL << pinReset_) | (1ULL << pinDc_);
    gpio_config(&io);

    io.mode = GPIO_MODE_INPUT;
    io.pin_bit_mask = (1ULL << pinBusy_);
    gpio_config(&io);

    ESP_LOGI(TAG, "Initialized with busy=%d, reset=%d, dc=%d",
             pinBusy_, pinReset_, pinDc_);

    return ESP_OK;
}

void EpaperDriver::reset() const {
    gpio_set_level(pinReset_, 0);
    vTaskDelay(pdMS_TO_TICKS(200));
    gpio_set_level(pinReset_, 1);
    vTaskDelay(pdMS_TO_TICKS(200));
}

void EpaperDriver::wait_ready() const {
    while (gpio_get_level(pinBusy_) == 1) {  // depends on panel
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void EpaperDriver::send_command(uint8_t cmd) const {
    gpio_set_level(pinDc_, 0);
    spi_transaction_t t = {};
    t.length = 8;
    t.tx_buffer = &cmd;
    spi_device_transmit(spi_, &t);
}

void EpaperDriver::send_data(const uint8_t* data, size_t len) const {
    gpio_set_level(pinDc_, 1);
    spi_transaction_t t = {};
    t.length = len * 8;
    t.tx_buffer = data;
    spi_device_transmit(spi_, &t);
}

void EpaperDriver::send_framebuffer(const uint8_t* buffer, size_t len) const {
    send_command(0x24);  // example: write RAM
    send_data(buffer, len);
    send_command(0x22);  // example: display update
    send_data((uint8_t*)"\xC7", 1);
    send_command(0x20);  // trigger update
    wait_ready();
}
