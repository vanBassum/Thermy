#pragma once
#include "Display.h"
#include "SSD1306.h"



class Display_SSD1306 : public Display
{
    inline static constexpr const char *TAG = "Display_SSD1306";
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

    void fill(uint8_t color) override { display.fill(color); }
    void drawPixel(int x, int y, bool color) override { display.drawPixel(x, y, color); }
    void show() override { display.show(); }
    void drawChar(int x, int y, char c, const TextStyle &style) override { display.drawChar(x, y, c, style); }
    void drawText(int x, int y, const char *str, const TextStyle &style) override { display.drawText(x, y, str, style); }

private:
    SSD1306_I2C display{OLED_WIDTH, OLED_HEIGHT, EXTERNAL_VCC};
};
