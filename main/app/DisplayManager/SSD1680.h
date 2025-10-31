#pragma once
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <vector>
#include <cstring>
#include <algorithm>

// --- SSD1680 command definitions ---
static constexpr uint8_t CMD_SW_RESET         = 0x12;
static constexpr uint8_t CMD_DRIVER_CONTROL   = 0x01;
static constexpr uint8_t CMD_DATA_ENTRY_MODE  = 0x11;
static constexpr uint8_t CMD_SET_RAMXPOS      = 0x44;
static constexpr uint8_t CMD_SET_RAMYPOS      = 0x45;
static constexpr uint8_t CMD_SET_RAMX_COUNTER = 0x4E;
static constexpr uint8_t CMD_SET_RAMY_COUNTER = 0x4F;
static constexpr uint8_t CMD_WRITE_BW_DATA    = 0x24;
static constexpr uint8_t CMD_MASTER_ACTIVATE  = 0x20;
static constexpr uint8_t CMD_UPDATE_DISPLAY   = 0x22;

// Your chosen logical orientation:
static constexpr uint16_t WIDTH  = 122;
static constexpr uint16_t HEIGHT = 250;

static constexpr uint16_t BYTES_PER_LINE = (WIDTH + 7) / 8;
static constexpr uint32_t FRAME_BYTES    = (uint32_t)BYTES_PER_LINE * HEIGHT;

// BUSY is usually active-low on SSD1680 modules
static constexpr bool BUSY_ACTIVE_LOW = true;

static gpio_num_t PIN_DC;
static gpio_num_t PIN_RST;
static gpio_num_t PIN_BUSY;

static bool is_busy_level(int lvl) {
    return BUSY_ACTIVE_LOW ? (lvl == 0) : (lvl == 1);
}

static bool wait_busy(uint32_t timeout_ms = 5000) {
    uint32_t waited = 0;
    // Wait until becomes busy (optional)
    if (!is_busy_level(gpio_get_level(PIN_BUSY))) {
        while (!is_busy_level(gpio_get_level(PIN_BUSY)) && waited < timeout_ms) {
            vTaskDelay(pdMS_TO_TICKS(1));
            ++waited;
        }
    }
    // Now wait until not busy
    waited = 0;
    while (is_busy_level(gpio_get_level(PIN_BUSY)) && waited < timeout_ms) {
        vTaskDelay(pdMS_TO_TICKS(1));
        ++waited;
    }
    return !is_busy_level(gpio_get_level(PIN_BUSY));
}

static void write_cmd(spi_device_handle_t spi, uint8_t cmd) {
    gpio_set_level(PIN_DC, 0);
    spi_transaction_t t = {};
    t.length = 8;
    t.tx_buffer = &cmd;
    ESP_ERROR_CHECK(spi_device_transmit(spi, &t));
}

static void write_data(spi_device_handle_t spi, const uint8_t *data, size_t len) {
    gpio_set_level(PIN_DC, 1);
    spi_transaction_t t = {};
    t.length = len * 8;
    t.tx_buffer = data;
    ESP_ERROR_CHECK(spi_device_transmit(spi, &t));
}

static void testingblabla(spi_device_handle_t spi, gpio_num_t pin_dc, gpio_num_t pin_rst, gpio_num_t pin_busy)
{
    PIN_DC = pin_dc;
    PIN_RST = pin_rst;
    PIN_BUSY = pin_busy;

    const uint16_t WIDTH = 122;
    const uint16_t HEIGHT = 250;
    const uint16_t BYTES_PER_LINE = (WIDTH + 7) / 8;
    const uint32_t FRAME_BYTES = (uint32_t)BYTES_PER_LINE * HEIGHT;

    // --- Reset ---
    gpio_set_level(PIN_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(PIN_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(10));

    // --- Software reset ---
    write_cmd(spi, 0x12);
    wait_busy();

    // --- Driver output control ---
    uint8_t driver_ctrl[3] = { (uint8_t)((HEIGHT - 1) & 0xFF), (uint8_t)(((HEIGHT - 1) >> 8) & 0xFF), 0x00 };
    write_cmd(spi, 0x01);
    write_data(spi, driver_ctrl, 3);

    // --- Booster soft start control ---
    uint8_t booster_softstart[4] = { 0xD7, 0xD6, 0x9D };
    write_cmd(spi, 0x0C);
    write_data(spi, booster_softstart, 3);

    // --- VCOM Voltage ---
    uint8_t vcom = 0xA8; // typical value
    write_cmd(spi, 0x2C);
    write_data(spi, &vcom, 1);

    // --- Dummy line period / Gate line width ---
    uint8_t dummy_line = 0x1A;
    uint8_t gate_width = 0x08;
    write_cmd(spi, 0x3A);
    write_data(spi, &dummy_line, 1);
    write_cmd(spi, 0x3B);
    write_data(spi, &gate_width, 1);

    // --- Data entry mode ---
    uint8_t data_entry = 0x03; // X+, Y+
    write_cmd(spi, 0x11);
    write_data(spi, &data_entry, 1);

    // --- Set RAM X range (byte units) ---
    // LilyGO T5 2.13" has 8-pixel offset: start at 1, end at (WIDTH/8 + 1)
    uint8_t x_range[2] = { 0x01, (uint8_t)(((WIDTH - 1) >> 3) + 1) };
    write_cmd(spi, 0x44);
    write_data(spi, x_range, 2);

    uint8_t y_range[4] = { 0x00, 0x00, (uint8_t)((HEIGHT - 1) & 0xFF), (uint8_t)(((HEIGHT - 1) >> 8) & 0xFF) };
    write_cmd(spi, 0x45);
    write_data(spi, y_range, 4);

    // --- Set RAM X counter (start address) ---
    uint8_t x_start = 0x01;
    write_cmd(spi, 0x4E);
    write_data(spi, &x_start, 1);
    write_cmd(spi, 0x4F);
    uint8_t yz[2] = { 0x00, 0x00 };
    write_data(spi, yz, 2);
    wait_busy();

    // --- Prepare framebuffer ---
    static uint8_t frame[FRAME_BYTES];
    memset(frame, 0xFF, sizeof(frame)); // white background

    auto Rect = [](int x0, int y0, int w, int h)
    {
      int x1 = std::min(x0 + w, (int)WIDTH);
      int y1 = std::min(y0 + h, (int)HEIGHT);
      for (int y = y0; y < y1; ++y) {
          for (int x = x0; x < x1; ++x) {
              uint32_t byte_index = (y * BYTES_PER_LINE) + (x >> 3);
              uint8_t bit_mask = 0x80 >> (x & 7);
              frame[byte_index] &= ~bit_mask; // black pixel = 0
          }
      }
    };

    // bottom-left corner
    Rect(0, 0, 1, 1);
    Rect(1, 1, 1, 1);
    Rect(2, 2, 1, 1);

    // bottom-right corner
    Rect(WIDTH - 1, 0, 1, 1);
    Rect(WIDTH - 2, 1, 1, 1);
    Rect(WIDTH - 3, 2, 1, 1);

    // top-left corner
    Rect(0, HEIGHT - 1, 1, 1);
    Rect(1, HEIGHT - 2, 1, 1);
    Rect(2, HEIGHT - 3, 1, 1);

    // top-right corner
    Rect(WIDTH - 1, HEIGHT - 1, 1, 1);
    Rect(WIDTH - 2, HEIGHT - 2, 1, 1);
    Rect(WIDTH - 3, HEIGHT - 3, 1, 1);


    Rect(10, 10, 10, 10);
    Rect(30, 10, 10, 10);
    Rect(10, 30, 10, 10);


    // --- Write black/white data ---
    write_cmd(spi, 0x24);
    write_data(spi, frame, FRAME_BYTES);

    // --- Display update control ---
    uint8_t display_update = 0xF7; // full refresh
    write_cmd(spi, 0x22);
    write_data(spi, &display_update, 1);
    write_cmd(spi, 0x20); // Master activate
    wait_busy(10000);

    // --- Deep sleep ---
    write_cmd(spi, 0x10);
    uint8_t sleep = 0x01;
    write_data(spi, &sleep, 1);

    ESP_LOGI("SSD1680", "Frame drawn successfully!");
}

