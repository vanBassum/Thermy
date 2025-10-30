#pragma once
#include "SSD1306.h"
#include "esp_log.h"
#include "EpaperDriver.h"

class Display
{
public:
    virtual void fill(uint8_t color) = 0;
    virtual void drawPixel(int x, int y, bool color) = 0;
    virtual void show() = 0;
    virtual void drawChar(int x, int y, char c, const TextStyle& style) = 0;
    virtual void drawText(int x, int y, const char *str, const TextStyle& style) = 0;
};

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

    void fill(uint8_t color) override    {        display.fill(color);    }
    void drawPixel(int x, int y, bool color) override    {        display.drawPixel(x, y, color);    }
    void show() override    {        display.show();    }
    void drawChar(int x, int y, char c, const TextStyle& style) override    {        display.drawChar(x, y, c, style);    }
    void drawText(int x, int y, const char *str, const TextStyle& style) override    {        display.drawText(x, y, str, style);    }
    
private:
    SSD1306_I2C display{OLED_WIDTH, OLED_HEIGHT, EXTERNAL_VCC};
};


class Display_SSD1680 : public Display {
    inline static constexpr const char *TAG = "DisplayDriver_SSD1680";

    // TTGO T5 V2.3.1 pins
    static constexpr spi_host_device_t EPAPER_HOST = SPI2_HOST;
    static constexpr gpio_num_t PIN_NUM_MOSI = GPIO_NUM_23;
    static constexpr gpio_num_t PIN_NUM_CLK  = GPIO_NUM_18;
    static constexpr gpio_num_t PIN_NUM_CS   = GPIO_NUM_5;
    static constexpr gpio_num_t PIN_NUM_DC   = GPIO_NUM_17;
    static constexpr gpio_num_t PIN_NUM_RST  = GPIO_NUM_16;
    static constexpr gpio_num_t PIN_NUM_BUSY = GPIO_NUM_4;

public:
    esp_err_t Init() {
        ESP_LOGI(TAG, "Initializing SSD1680 display...");

        // --- SPI bus setup ---
        spi_bus_config_t buscfg = {};
        buscfg.miso_io_num = -1;
        buscfg.mosi_io_num = PIN_NUM_MOSI;
        buscfg.sclk_io_num = PIN_NUM_CLK;
        buscfg.max_transfer_sz = 4096;
        buscfg.flags = SPICOMMON_BUSFLAG_MASTER;

        ESP_ERROR_CHECK(spi_bus_initialize(EPAPER_HOST, &buscfg, SPI_DMA_CH_AUTO));

        // --- SPI device setup ---
        spi_device_interface_config_t devcfg = {};
        devcfg.mode = 0;
        devcfg.clock_speed_hz = 4 * 1000 * 1000;
        devcfg.spics_io_num = PIN_NUM_CS;
        devcfg.queue_size = 3;

        ESP_ERROR_CHECK(spi_bus_add_device(EPAPER_HOST, &devcfg, &spi_));

        // --- EPD driver init ---
        ESP_ERROR_CHECK(epaper.init(spi_, PIN_NUM_BUSY, PIN_NUM_RST, PIN_NUM_DC));
        epaper.reset();

        ESP_LOGI(TAG, "SSD1680 display initialized successfully.");
        return ESP_OK;
    }

    void fill(uint8_t color) override {}
    void drawPixel(int x, int y, bool color) override {}

    void show() override {
        ESP_LOGI(TAG, "Displaying test pattern...");
        // Example: send checkerboard pattern to SSD1680 RAM
        constexpr size_t WIDTH = 212;
        constexpr size_t HEIGHT = 104;
        constexpr size_t BYTES = (WIDTH * HEIGHT) / 8;
        static uint8_t framebuffer[BYTES];

        // Create simple pattern
        for (size_t i = 0; i < BYTES; ++i) {
            framebuffer[i] = (i & 1) ? 0xAA : 0x55;  // alternating pattern
        }

        // SSD1680 typical sequence
        epaper.send_command(0x24);  // write RAM black/white
        epaper.send_data(framebuffer, BYTES);

        // Trigger display refresh
        epaper.send_command(0x22);  // display update control
        uint8_t data = 0xC7;
        epaper.send_data(&data, 1);
        epaper.send_command(0x20);  // master activation
        epaper.wait_ready();

        ESP_LOGI(TAG, "Pattern displayed.");
    }

    void drawChar(int x, int y, char c, const TextStyle& style) override {}
    void drawText(int x, int y, const char *str, const TextStyle& style) override {}

private:
    EpaperDriver epaper;
    spi_device_handle_t spi_ = nullptr;
};
