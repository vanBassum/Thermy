#pragma once
#include "lvgl.h"
#include "driver/spi_master.h"
#include "driver/ledc.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"
#include "esp_timer.h"

class Display_WT32SC01
{
    inline static constexpr const char *TAG = "Display_WT32SC01";

    static constexpr int LCD_MOSI = 23;
    static constexpr int LCD_CLK  = 18;
    static constexpr int LCD_CS   = 5;
    static constexpr int LCD_DC   = 27;
    static constexpr int LCD_RST  = 33;
    static constexpr int LCD_BL   = 32;
    static constexpr int LCD_HRES = 480;
    static constexpr int LCD_VRES = 320;

    esp_lcd_panel_handle_t panel = nullptr;
    esp_timer_handle_t lvTickTimer = nullptr;
    lv_disp_draw_buf_t drawBuf;
    lv_color_t *buf1 = nullptr;
    lv_color_t *buf2 = nullptr;

public:
    Display_WT32SC01() = default;
    ~Display_WT32SC01();

    void Init();
    void Tick();
    void Update();
    void SetBrightness(uint8_t percent);

private:
    void InitBacklight();
    static void LvglFlushCb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p);
    static void LvglTickCb(void *arg);
};
