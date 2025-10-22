#include "ssd1306.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include <algorithm>
#include "esp_log.h"
#include "font5x7.h"


// ==== SSD1306 Command Constants ====
constexpr uint8_t SET_CONTRAST        = 0x81;
constexpr uint8_t SET_ENTIRE_ON       = 0xA4;
constexpr uint8_t SET_NORM_INV        = 0xA6;
constexpr uint8_t SET_DISP            = 0xAE;
constexpr uint8_t SET_MEM_ADDR        = 0x20;
constexpr uint8_t SET_COL_ADDR        = 0x21;
constexpr uint8_t SET_PAGE_ADDR       = 0x22;
constexpr uint8_t SET_DISP_START_LINE = 0x40;
constexpr uint8_t SET_SEG_REMAP       = 0xA0;
constexpr uint8_t SET_MUX_RATIO       = 0xA8;
constexpr uint8_t SET_COM_OUT_DIR     = 0xC0;
constexpr uint8_t SET_DISP_OFFSET     = 0xD3;
constexpr uint8_t SET_COM_PIN_CFG     = 0xDA;
constexpr uint8_t SET_DISP_CLK_DIV    = 0xD5;
constexpr uint8_t SET_PRECHARGE       = 0xD9;
constexpr uint8_t SET_VCOM_DESEL      = 0xDB;
constexpr uint8_t SET_CHARGE_PUMP     = 0x8D;

SSD1306::SSD1306(uint8_t w, uint8_t h, bool ext_vcc)
    : width(w), height(h), external_vcc(ext_vcc)
{
    pages = height / 8;
    buffer = new uint8_t[pages * width];
    memset(buffer, 0, pages * width);
}

SSD1306::~SSD1306() {
    delete[] buffer;
}

void SSD1306::initDisplay() {
    const uint8_t cmds[] = {
        SET_DISP | 0x00,
        SET_MEM_ADDR, 0x00,
        SET_DISP_START_LINE | 0x00,
        SET_SEG_REMAP | 0x01,
        SET_MUX_RATIO, (uint8_t)(height - 1),
        SET_COM_OUT_DIR | 0x08,
        SET_DISP_OFFSET, 0x00,
        SET_COM_PIN_CFG, (uint8_t)((width > 2 * height) ? 0x02 : 0x12),
        SET_DISP_CLK_DIV, 0x80,
        SET_PRECHARGE, (uint8_t)(external_vcc ? 0x22 : 0xF1),
        SET_VCOM_DESEL, 0x30,
        SET_CONTRAST, 0xFF,
        SET_ENTIRE_ON,
        SET_NORM_INV,
        SET_CHARGE_PUMP, (uint8_t)(external_vcc ? 0x10 : 0x14),
        SET_DISP | 0x01
    };
    for (uint8_t c : cmds) writeCmd(c);
    fill(0);
    show();
}

void SSD1306::powerOff() { writeCmd(SET_DISP | 0x00); }
void SSD1306::powerOn()  { writeCmd(SET_DISP | 0x01); }

void SSD1306::contrast(uint8_t value) {
    writeCmd(SET_CONTRAST);
    writeCmd(value);
}

void SSD1306::invert(bool inv) {
    writeCmd(SET_NORM_INV | (inv ? 1 : 0));
}

void SSD1306::fill(uint8_t color) {
    memset(buffer, color ? 0xFF : 0x00, pages * width);
}

void SSD1306::drawPixel(int x, int y, bool color) {
    if (x < 0 || x >= width || y < 0 || y >= height) return;
    int page = y / 8;
    int bit = y % 8;
    if (color)
        buffer[x + page * width] |= (1 << bit);
    else
        buffer[x + page * width] &= ~(1 << bit);
}

void SSD1306::show() {
    // These offsets align the 128×64 buffer to the visible 72×40 window
    const uint8_t xOffset = 26;  // shift right (try 26–32)
    const uint8_t yOffset = 3;   // shift down (3 pages × 8 px = 24 px)

    for (uint8_t page = 0; page < pages; page++) {
        writeCmd(0xB0 + page + yOffset);        // set page address (vertical offset)
        writeCmd((xOffset + 2) & 0x0F);         // lower column start
        writeCmd(0x10 | ((xOffset + 2) >> 4));  // higher column start
        writeData(&buffer[page * width], width);
    }
}


void SSD1306::drawChar(int x, int y, char c, int size) {
    if (c < 32 || c > 127) return;
    const uint8_t* glyph = font5x7[c - 32];

    for (int col = 0; col < 5; ++col) {
        uint8_t bits = glyph[col];
        for (int row = 0; row < 7; ++row) {
            if (bits & (1 << row)) {
                // Scale both width and height by "size"
                for (int dx = 0; dx < size; ++dx) {
                    for (int dy = 0; dy < size; ++dy) {
                        drawPixel(x + col * size + dx, y + row * size + dy, true);
                    }
                }
            }
        }
    }

    // Optional: draw a column of spacing pixels at the end
    if (size > 1) {
        int spacing = size; // keep proportional
        for (int dx = 0; dx < spacing; ++dx) {
            for (int dy = 0; dy < 7 * size; ++dy) {
                drawPixel(x + 5 * size + dx, y + dy, false);
            }
        }
    }
}

void SSD1306::drawText(int x, int y, const char* str, int size) {
    while (*str) {
        drawChar(x, y, *str++, size);
        x += 6 * size; // 5 px glyph + 1 px spacing
    }
}





// ===================== I2C =====================

SSD1306_I2C::SSD1306_I2C(uint8_t w, uint8_t h, i2c_port_t i2c_port,
                         uint8_t addr, bool ext_vcc)
    : SSD1306(w, h, ext_vcc), port(i2c_port), address(addr)
{
    initDisplay();
}

// --- writeCmd: one short transaction (START -> 0x3C + W -> 0x80 -> cmd -> STOP)
void SSD1306_I2C::writeCmd(uint8_t cmd) {
    i2c_cmd_handle_t c = i2c_cmd_link_create();
    i2c_master_start(c);
    i2c_master_write_byte(c, (address << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(c, 0x80, true);        // control: Co=1, D/C#=0
    i2c_master_write_byte(c, cmd, true);
    i2c_master_stop(c);
    esp_err_t err = i2c_master_cmd_begin(port, c, pdMS_TO_TICKS(50));
    i2c_cmd_link_delete(c);
    if (err != ESP_OK) ESP_LOGW(OLED_TAG, "writeCmd 0x%02X err=%d", cmd, (int)err);
}

void SSD1306_I2C::writeData(const uint8_t* data, size_t len) {
    constexpr size_t CHUNK = 32;  // safe size for ESP-IDF I2C
    while (len > 0) {
        size_t n = len > CHUNK ? CHUNK : len;

        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_WRITE, true);
        i2c_master_write_byte(cmd, 0x40, true);  // control byte: Co=0, D/C#=1
        i2c_master_write(cmd, data, n, true);
        i2c_master_stop(cmd);

        esp_err_t err = i2c_master_cmd_begin(port, cmd, pdMS_TO_TICKS(100));
        i2c_cmd_link_delete(cmd);

        if (err != ESP_OK) {
            ESP_LOGW("SSD1306", "writeData chunk err=%d", (int)err);
            break;
        }

        data += n;
        len -= n;

        vTaskDelay(0);  // yield so IDLE task runs (prevents watchdog)
    }
}


// ===================== SPI =====================

SSD1306_SPI::SSD1306_SPI(uint8_t w, uint8_t h, spi_device_handle_t dev,
                         gpio_num_t dc, gpio_num_t rst, bool ext_vcc)
    : SSD1306(w, h, ext_vcc), spi(dev), dc_pin(dc), rst_pin(rst)
{
    gpio_set_direction(dc_pin, GPIO_MODE_OUTPUT);
    gpio_set_direction(rst_pin, GPIO_MODE_OUTPUT);

    gpio_set_level(rst_pin, 1);
    vTaskDelay(pdMS_TO_TICKS(1));
    gpio_set_level(rst_pin, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(rst_pin, 1);

    initDisplay();
}

void SSD1306_SPI::writeCmd(uint8_t cmd) {
    gpio_set_level(dc_pin, 0);
    spi_transaction_t t = {};
    t.length = 8;
    t.tx_buffer = &cmd;
    spi_device_transmit(spi, &t);
}

void SSD1306_SPI::writeData(const uint8_t* data, size_t len) {
    gpio_set_level(dc_pin, 1);
    spi_transaction_t t = {};
    t.length = len * 8;
    t.tx_buffer = data;
    spi_device_transmit(spi, &t);
}
