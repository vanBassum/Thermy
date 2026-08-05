#pragma once

#include "BoardConfig.h"
#include "lvgl.h"
#include "driver/spi_master.h"
#include "driver/ledc.h"
#include "driver/i2c_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_touch.h"
#include "esp_log.h"
#include "esp_timer.h"

// ──────────────────────────────────────────────────────────────
// The WT32-SC01's panel: an ST7796 480x320 LCD on SPI2 with an FT6336
// capacitive touch controller on I2C0, both registered with LVGL.
//
// Pins come from BoardConfig — this driver names no GPIO of its own, so the
// board folder stays the single place the wiring is written down.
//
// Touch is optional at runtime: if the controller does not answer, the
// display still comes up and Init() logs a warning rather than aborting. A
// thermostat that can be read but not touched is far more useful than one
// that boot-loops.
// ──────────────────────────────────────────────────────────────

class Display_WT32SC01
{
    inline static constexpr const char *TAG = "Display_WT32SC01";

public:
    static constexpr int LCD_HRES = BoardConfig::LCD_HRES;
    static constexpr int LCD_VRES = BoardConfig::LCD_VRES;

    Display_WT32SC01() = default;
    ~Display_WT32SC01();

    Display_WT32SC01(const Display_WT32SC01 &) = delete;
    Display_WT32SC01 &operator=(const Display_WT32SC01 &) = delete;

    void Init();
    void SetBrightness(uint8_t percent);

    lv_disp_t *GetLvglDisplay() const { return disp; }
    bool HasTouch() const { return touch != nullptr; }

private:
    // --- Display ---
    esp_lcd_panel_handle_t panel = nullptr;
    lv_disp_draw_buf_t drawBuf;
    lv_color_t *buf1 = nullptr;
    lv_color_t *buf2 = nullptr;
    lv_disp_drv_t dispDrv;
    lv_disp_t *disp = nullptr;

    // --- Touch ---
    i2c_master_bus_handle_t i2cBus = nullptr;
    esp_lcd_touch_handle_t touch = nullptr;
    lv_indev_t *inputDev = nullptr;

    // --- Methods ---
    void InitBacklight();
    void InitTouch();
    static void LvglFlushCb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p);
    static void LvglTouchCb(lv_indev_drv_t *drv, lv_indev_data_t *data);
};
