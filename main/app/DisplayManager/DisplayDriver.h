#pragma once
#include "SSD1306.h"
#include "esp_log.h"

class DisplayDriver
{
    inline static constexpr const char *TAG = "DisplayDriver";
    constexpr static const uint8_t OLED_WIDTH = 128;
    constexpr static const uint8_t OLED_HEIGHT = 64;
    constexpr static const uint8_t OLED_ADDR = 0x3C;
    constexpr static const bool EXTERNAL_VCC = false;

public:
    esp_err_t Init(i2c_master_bus_handle_t bus)
    {
        esp_err_t err = display.Init(bus, OLED_ADDR);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to initialize SSD1306: %s", esp_err_to_name(err));
            return err;
        }

        display.fill(0);
        display.show();
        ESP_LOGI(TAG, "Display initialized successfully.");
        return ESP_OK;
    }

    SSD1306 &GetDisplay()
    {
        return display;
    }

private:
    SSD1306_I2C display{OLED_WIDTH, OLED_HEIGHT, EXTERNAL_VCC};
};