#pragma once
#include <stdint.h>
#include <string.h>
#include "driver/i2c.h"
#include "driver/spi_master.h"
#include "esp_err.h"

// https://emalliab.wordpress.com/2025/02/12/esp32-c3-0-42-oled/
// https://electronics.stackexchange.com/questions/725871/how-to-use-onboard-0-42-inch-oled-for-esp32-c3-oled-development-board-with-micro
// https://raw.githubusercontent.com/stlehmann/micropython-ssd1306/refs/heads/master/ssd1306.py

// ==========================================================
class SSD1306
{
public:
    SSD1306(uint8_t width, uint8_t height, bool external_vcc);
    virtual ~SSD1306();

    void initDisplay();
    void powerOff();
    void powerOn();
    void contrast(uint8_t value);
    void invert(bool invert);
    void fill(uint8_t color);
    void drawPixel(int x, int y, bool color);
    void show();
    void drawChar(int x, int y, char c, int size=1);
    void drawText(int x, int y, const char *str, int size=1);

protected:
    virtual void writeCmd(uint8_t cmd) = 0;
    virtual void writeData(const uint8_t *data, size_t len) = 0;

    uint8_t width;
    uint8_t height;
    bool external_vcc;
    uint8_t pages;
    uint8_t *buffer;
};

// ==========================================================
class SSD1306_I2C : public SSD1306
{
    inline static constexpr const char *OLED_TAG = "SSD1306_I2C";

public:
    SSD1306_I2C(uint8_t width, uint8_t height,
                i2c_port_t i2c_port, uint8_t addr = 0x3C,
                bool external_vcc = false);
    ~SSD1306_I2C() override = default;

protected:
    void writeCmd(uint8_t cmd) override;
    void writeData(const uint8_t *data, size_t len) override;

private:
    i2c_port_t port;
    uint8_t address;
};

// ==========================================================
class SSD1306_SPI : public SSD1306
{
public:
    SSD1306_SPI(uint8_t width, uint8_t height,
                spi_device_handle_t spi,
                gpio_num_t dc, gpio_num_t rst,
                bool external_vcc = false);
    ~SSD1306_SPI() override = default;

protected:
    void writeCmd(uint8_t cmd) override;
    void writeData(const uint8_t *data, size_t len) override;

private:
    spi_device_handle_t spi;
    gpio_num_t dc_pin;
    gpio_num_t rst_pin;
};
