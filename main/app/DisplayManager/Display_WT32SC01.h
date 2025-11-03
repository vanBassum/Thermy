#pragma once
#include "lvgl.h"
#include "driver/spi_master.h"
#include "driver/ledc.h"
#include "driver/i2c.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_touch_gt911.h"
#include "esp_log.h"
#include "esp_timer.h"

class Display_WT32SC01
{
    inline static constexpr const char *TAG = "Display_WT32SC01";

    // --- LCD pins and parameters ---
    static constexpr int LCD_MOSI = 13;
    static constexpr int LCD_CLK  = 14;
    static constexpr int LCD_CS   = 15;
    static constexpr int LCD_DC   = 21;
    static constexpr int LCD_RST  = 22;
    static constexpr int LCD_BL   = 23;

    // --- Touch (GT911) pins ---
    static constexpr int TOUCH_SDA = 18; 
    static constexpr int TOUCH_SCL = 19; 
    static constexpr int TOUCH_RST = 25; 
    static constexpr int TOUCH_INT = 39; 

public:
    static constexpr int LCD_HRES = 480;
    static constexpr int LCD_VRES = 320;

    Display_WT32SC01() = default;
    ~Display_WT32SC01();

    void Init();
    void SetBrightness(uint8_t percent);
    lv_disp_t* GetLvglDisplay() const { return disp; }

private:
    // --- Display ---
    esp_lcd_panel_handle_t panel = nullptr;
    lv_disp_draw_buf_t drawBuf;
    lv_color_t *buf1 = nullptr;
    lv_color_t *buf2 = nullptr;
    lv_disp_drv_t dispDrv;
    lv_disp_t *disp = nullptr;

    // --- Touch ---
    esp_lcd_touch_handle_t touch = nullptr;
    lv_indev_t *inputDev = nullptr;

    // --- Methods ---
    void InitBacklight();
    void InitTouch();
    static void LvglFlushCb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p);
    static void LvglTouchCb(lv_indev_drv_t *drv, lv_indev_data_t *data);
};
