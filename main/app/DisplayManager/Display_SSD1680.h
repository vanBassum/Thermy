#pragma once
#include "Display.h"
#include "SSD1680.h"


class Display_SSD1680 : public Display
{
    inline static constexpr const char *TAG = "DisplayDriver_SSD1680";

    // TTGO T5 V2.3.1 pins
    static constexpr spi_host_device_t EPAPER_HOST = SPI2_HOST;
    static constexpr gpio_num_t PIN_NUM_MOSI = GPIO_NUM_23;
    static constexpr gpio_num_t PIN_NUM_CLK = GPIO_NUM_18;
    static constexpr gpio_num_t PIN_NUM_CS = GPIO_NUM_5;
    static constexpr gpio_num_t PIN_NUM_DC = GPIO_NUM_17;
    static constexpr gpio_num_t PIN_NUM_RST = GPIO_NUM_16;
    static constexpr gpio_num_t PIN_NUM_BUSY = GPIO_NUM_4;


    SSD1680 driver;
    spi_device_handle_t spi = nullptr;
    uint8_t frameBuffer[SSD1680::FRAME_BYTES] = {0};

public:
    esp_err_t Init()
    {
        ESP_LOGI(TAG, "Initializing SSD1680 display...");

        // --- GPIOs ---
        gpio_set_direction(PIN_NUM_DC, GPIO_MODE_OUTPUT);
        gpio_set_direction(PIN_NUM_RST, GPIO_MODE_OUTPUT);
        gpio_set_direction(PIN_NUM_BUSY, GPIO_MODE_INPUT);

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
        ESP_ERROR_CHECK(spi_bus_add_device(EPAPER_HOST, &devcfg, &spi));
        
        driver.Init(spi, PIN_NUM_DC, PIN_NUM_RST, PIN_NUM_BUSY);
        return ESP_OK;
    }

    void fill(uint8_t color) override
    {
        // color: 1 = black, 0 = white
        std::memset(frameBuffer, color ? 0x00 : 0xFF, sizeof(frameBuffer));
    }

    void drawPixel(int x, int y, bool color) override
    {
        if (x < 0 || x >= SSD1680::WIDTH || y < 0 || y >= SSD1680::HEIGHT)
            return;

        // --- Handle bottom-lef origin ---
        int hwX = x;
        int hwY = (SSD1680::HEIGHT - 1) - y;

        uint32_t byteIndex = hwY * SSD1680::BYTES_PER_LINE + (hwX >> 3);
        uint8_t bitMask = 0x80 >> (hwX & 7);

        if (color)
            frameBuffer[byteIndex] &= ~bitMask; // black pixel = 0
        else
            frameBuffer[byteIndex] |= bitMask;  // white pixel = 1
            
    }

    void show() override
    {
        driver.WriteFrame(frameBuffer);
        driver.Update();
    }


    void drawChar(int x, int y, char c, const TextStyle& style)
    {
        if (!style.font) return;

        const uint8_t* glyph = style.font->GetGlyph(c);
        if (!glyph) return;

        for (int col = 0; col < style.font->width; ++col)
        {
            uint8_t bits = glyph[col];
            for (int row = 0; row < style.font->height; ++row)
            {
                if (bits & (1 << row))
                {
                    for (int dx = 0; dx < style.size; ++dx)
                    {
                        for (int dy = 0; dy < style.size; ++dy)
                        {
                            drawPixel(
                                x + col * style.size + dx,
                                y + row * style.size + dy,
                                style.color
                            );
                        }
                    }
                }
            }
        }
    }

    void drawText(int x, int y, const char* str, const TextStyle& style)
    {
        if (!style.font || !str) return;

        int cursorX = x;
        while (*str)
        {
            drawChar(cursorX, y, *str++, style);
            cursorX += (style.font->width + 1) * style.size;
        }
    }
};

