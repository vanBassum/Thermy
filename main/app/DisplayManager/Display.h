#pragma once
#include "SSD1306.h"
#include "SSD1680.h"
#include "esp_log.h"

class Display
{
public:
    virtual void fill(uint8_t color) = 0;
    virtual void drawPixel(int x, int y, bool color) = 0;
    virtual void show() = 0;
    virtual void drawChar(int x, int y, char c, const TextStyle &style) = 0;
    virtual void drawText(int x, int y, const char *str, const TextStyle &style) = 0;
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

    void fill(uint8_t color) override { display.fill(color); }
    void drawPixel(int x, int y, bool color) override { display.drawPixel(x, y, color); }
    void show() override { display.show(); }
    void drawChar(int x, int y, char c, const TextStyle &style) override { display.drawChar(x, y, c, style); }
    void drawText(int x, int y, const char *str, const TextStyle &style) override { display.drawText(x, y, str, style); }

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

    // SSD1680 / 2.13" ePaper resolution
    static constexpr int EPD_WIDTH  = 256;
    static constexpr int EPD_HEIGHT = 122;

    // SSD1680 RAM must be addressed in 8-pixel wide chunks
    static constexpr uint8_t width_bytes = (EPD_WIDTH + 7) / 8;
    static constexpr size_t buf_size = width_bytes * EPD_HEIGHT;

    // Create a frame buffer filled with 0x00 (black)
    uint8_t frame[buf_size] = {0};

    spi_device_handle_t spi_;
    SSD1680_HandleTypeDef epd_;

public:
    esp_err_t Init() {
        ESP_LOGI(TAG, "Initializing SSD1680 display...");

        // --- GPIO setup
        gpio_config_t io_conf = {};
        io_conf.mode = GPIO_MODE_OUTPUT;
        io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
        io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
        io_conf.intr_type = GPIO_INTR_DISABLE;
        io_conf.pin_bit_mask = (1ULL << PIN_NUM_DC) | (1ULL << PIN_NUM_RST);
        ESP_ERROR_CHECK(gpio_config(&io_conf));

        // Busy pin (input)
        gpio_config_t busy_conf = {};
        busy_conf.mode = GPIO_MODE_INPUT;
        busy_conf.pin_bit_mask = (1ULL << PIN_NUM_BUSY);
        busy_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
        busy_conf.pull_up_en = GPIO_PULLUP_DISABLE;
        busy_conf.intr_type = GPIO_INTR_DISABLE;
        ESP_ERROR_CHECK(gpio_config(&busy_conf));

        // --- SPI setup
        spi_bus_config_t buscfg = {};
        buscfg.mosi_io_num = PIN_NUM_MOSI;
        buscfg.miso_io_num = -1;
        buscfg.sclk_io_num = PIN_NUM_CLK;
        buscfg.quadwp_io_num = -1;
        buscfg.quadhd_io_num = -1;
        buscfg.max_transfer_sz = 4096;
        ESP_ERROR_CHECK(spi_bus_initialize(EPAPER_HOST, &buscfg, SPI_DMA_CH_AUTO));

        spi_device_interface_config_t devcfg = {};
        devcfg.mode = 0;
        devcfg.clock_speed_hz = 4 * 1000 * 1000;
        devcfg.spics_io_num = PIN_NUM_CS;
        devcfg.queue_size = 3;
        ESP_ERROR_CHECK(spi_bus_add_device(EPAPER_HOST, &devcfg, &spi_));

        // --- SSD1680 handle setup
        epd_.spi = spi_;
        epd_.spi_timeout_ms = 1000;
        epd_.cs_pin = PIN_NUM_CS;
        epd_.dc_pin = PIN_NUM_DC;
        epd_.reset_pin = PIN_NUM_RST;
        epd_.busy_pin = PIN_NUM_BUSY;
        epd_.Color_Depth = 1; // black/white
        epd_.Scan_Mode = NarrowScan;
        epd_.Resolution_X = EPD_WIDTH;
        epd_.Resolution_Y = EPD_HEIGHT;

        // --- Initialize and show pattern
        SSD1680_Init(&epd_);
        SSD1680_Clear(&epd_, ColorWhite);
        SSD1680_Checker(&epd_);
        SSD1680_Refresh(&epd_, FullRefresh);

        ESP_LOGI(TAG, "SSD1680 display initialized successfully.");
        return ESP_OK;
    }

    void fill(uint8_t color) override {
        SSD1680_Clear(&epd_, color ? ColorBlack : ColorWhite);
    }

    void drawPixel(int x, int y, bool color) override {
        // not implemented for this test
    }

    void show() override {
        return;
        ESP_LOGI(TAG, "Displaying solid black frame (%dx%d)...", EPD_WIDTH, EPD_HEIGHT);

        
        // Write frame to the visible region
        esp_err_t err = SSD1680_SetRegion(&epd_, 0, 0, EPD_WIDTH, EPD_HEIGHT, frame, nullptr);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "SSD1680_SetRegion failed: %d", err);
            return;
        }

        // Trigger display refresh
        SSD1680_Refresh(&epd_, FullRefresh);
        ESP_LOGI(TAG, "Solid black frame displayed.");
    }

    void drawChar(int x, int y, char c, const TextStyle& style) override {}
    void drawText(int x, int y, const char *str, const TextStyle& style) override {}
};

