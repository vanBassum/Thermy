#pragma once
#include "SSD1306.h"

class DisplayDriver {
    constexpr static const i2c_port_t i2c_port = I2C_NUM_0;
    constexpr static const gpio_num_t sda_gpio = GPIO_NUM_5;
    constexpr static const gpio_num_t scl_gpio = GPIO_NUM_6;
    constexpr static const uint32_t i2c_freq = 400000;
    constexpr static const uint8_t oled_width = 128;
    constexpr static const uint8_t oled_height = 64;
    constexpr static const uint8_t oled_addr = 0x3C;
    constexpr static const bool external_vcc = false;

public:
    esp_err_t Init() {
        i2c_config_t conf = {};
        conf.mode = I2C_MODE_MASTER;
        conf.sda_io_num = sda_gpio;
        conf.scl_io_num = scl_gpio;
        conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
        conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
        conf.master.clk_speed = i2c_freq;

        ESP_ERROR_CHECK(i2c_param_config(i2c_port, &conf));
        ESP_ERROR_CHECK(i2c_driver_install(i2c_port, conf.mode, 0, 0, 0));

        display.initDisplay();
        display.contrast(0xFF);
        display.fill(0);
        display.show();
        return ESP_OK;
    }

    SSD1306& GetDisplay() {
        return display;
    }

private:
    SSD1306_I2C display{oled_width, oled_height, i2c_port, oled_addr, external_vcc};
};
