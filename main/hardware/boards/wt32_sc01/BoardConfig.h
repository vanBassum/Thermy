#pragma once

#include "driver/gpio.h"

// ──────────────────────────────────────────────────────────────
// Board configuration — WT32-SC01 (ESP32-WROVER-B, 3.5" 480x320
// ST7796 SPI LCD with an FT6336 capacitive touch panel).
//
// Pin assignments and constants for this board only. Other boards live in
// sibling folders under hardware/boards/ and are selected with -DBOARD=<name>.
//
// Everything the panel and the 1-Wire bus are wired to lives here, so the
// board folder is the single place a pin is written down. The panel driver
// (Display_WT32SC01) reads these constants rather than defining its own.
// ──────────────────────────────────────────────────────────────

namespace BoardConfig
{
    // ── LED ──────────────────────────────────────────────────
    // The WT32-SC01 exposes no user LED — the backlight is the only
    // controllable light and it belongs to the panel. Board binds
    // MockLed so application code that speaks the Led role still
    // compiles and links (see hardware/interfaces/Led.h).

    // ── LCD (ST7796 over SPI2) ───────────────────────────────
    static constexpr gpio_num_t LCD_MOSI = GPIO_NUM_13;
    static constexpr gpio_num_t LCD_CLK  = GPIO_NUM_14;
    static constexpr gpio_num_t LCD_CS   = GPIO_NUM_15;
    static constexpr gpio_num_t LCD_DC   = GPIO_NUM_21;
    static constexpr gpio_num_t LCD_RST  = GPIO_NUM_22;
    static constexpr gpio_num_t LCD_BL   = GPIO_NUM_23;

    static constexpr int LCD_HRES = 480;
    static constexpr int LCD_VRES = 320;

    // ── Touch (FT6336 over I2C0) ─────────────────────────────
    static constexpr gpio_num_t TOUCH_SDA = GPIO_NUM_18;
    static constexpr gpio_num_t TOUCH_SCL = GPIO_NUM_19;
    static constexpr gpio_num_t TOUCH_INT = GPIO_NUM_39;

    // ── 1-Wire (DS18B20 temperature probes) ──────────────────
    // The bus host is created by Board and handed to SensorManager, so the
    // application never names a GPIO.
    static constexpr gpio_num_t ONEWIRE_PIN = GPIO_NUM_4;

    // How many probes the bus is expected to carry. Sizes the board's
    // rom-search result buffer; SensorManager has its own slot count.
    static constexpr int ONEWIRE_MAX_DEVICES = 8;
}
