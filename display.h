#pragma once
#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "driver/i2c.h"


constexpr const char *TAG = "I2C_OLED_Test";

#define I2C_MASTER_NUM      I2C_NUM_1
#define I2C_MASTER_SCL_IO   4
#define I2C_MASTER_SDA_IO   5
#define I2C_MASTER_FREQ_HZ  400000
#define OLED_ADDR           0x3C

// This does something, but the screen om my board seems broken.



// Initialize the I2C bus
void i2c_master_init() {
    i2c_config_t conf = {};
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_MASTER_SDA_IO;
    conf.scl_io_num = I2C_MASTER_SCL_IO;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;

    ESP_ERROR_CHECK(i2c_param_config(I2C_MASTER_NUM, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0));

    printf("Scanning I2C bus...\n");

    uint8_t dummy = 0;
    for (uint8_t addr = 1; addr < 127; addr++) {
        esp_err_t res = i2c_master_write_to_device(I2C_MASTER_NUM, addr, &dummy, 1, pdMS_TO_TICKS(10));
        if (res == ESP_OK) {
            printf("✅ Found device at 0x%02X\n", addr);
        }
    }

    printf("Scan complete.\n");
}

static esp_err_t oled_cmd(uint8_t cmd) {
    uint8_t buf[2] = {0x00, cmd};
    return i2c_master_write_to_device(I2C_MASTER_NUM, OLED_ADDR, buf, 2, pdMS_TO_TICKS(100));
}

static esp_err_t oled_data(uint8_t data) {
    uint8_t buf[2] = {0x40, data};
    return i2c_master_write_to_device(I2C_MASTER_NUM, OLED_ADDR, buf, 2, pdMS_TO_TICKS(100));
}
void Test() {
    ESP_LOGI(TAG, "Starting OLED test...");

    auto cmd = [](uint8_t c) {
        uint8_t buf[2] = {0x00, c};
        i2c_master_write_to_device(I2C_MASTER_NUM, OLED_ADDR, buf, 2, pdMS_TO_TICKS(100));
    };
    auto data = [](uint8_t d) {
        uint8_t buf[2] = {0x40, d};
        i2c_master_write_to_device(I2C_MASTER_NUM, OLED_ADDR, buf, 2, pdMS_TO_TICKS(100));
    };

    // Try both power configurations: external and internal booster
    for (int mode = 0; mode < 2; mode++) {
        ESP_LOGI(TAG, "Testing OLED mode: %s", mode ? "internal charge pump" : "external VCC");

        cmd(0xAE); // Display OFF
        cmd(0xD5); cmd(0x80);
        cmd(0xA8); cmd(0x3F);
        cmd(0xD3); cmd(0x00);
        cmd(0x40);
        cmd(0x8D); cmd(mode ? 0x14 : 0x10);  // 0x14 = internal, 0x10 = external
        cmd(0x20); cmd(0x00);                // horizontal addressing
        cmd(0xA1);
        cmd(0xC8);
        cmd(0xDA); cmd(0x12);
        cmd(0x81); cmd(0xFF);
        cmd(0xD9); cmd(0xF1);
        cmd(0xDB); cmd(0x40);
        cmd(0xA4);
        cmd(0xA6);
        cmd(0xAF); // Display ON

        vTaskDelay(pdMS_TO_TICKS(200));

        // Fill with a visible pattern
        for (int page = 0; page < 8; page++) {
            cmd(0xB0 + page);
            cmd(0x00);
            cmd(0x10);
            for (int col = 0; col < 128; col++) {
                data((page % 2) ? 0xFF : 0x00);
            }
        }

        ESP_LOGI(TAG, "Pattern written for mode %d. Should show white/black stripes.", mode);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }

    ESP_LOGI(TAG, "OLED test complete.");
}



