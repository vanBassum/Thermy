#include "Display_WT32SC01.h"
#include "esp_lcd_st7796.h"
#include <cassert>

Display_WT32SC01::~Display_WT32SC01()
{
    if (panel)
        esp_lcd_panel_del(panel);
    free(buf1);
    free(buf2);
}

void Display_WT32SC01::Init()
{
    ESP_LOGI(TAG, "Initializing WT32-SC01 display");

    // --- SPI bus
    spi_bus_config_t buscfg = {};
    buscfg.sclk_io_num = LCD_CLK;
    buscfg.mosi_io_num = LCD_MOSI;
    buscfg.miso_io_num = -1;
    buscfg.max_transfer_sz = LCD_HRES * 40 * sizeof(uint16_t);
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));

    // --- SPI IO
    esp_lcd_panel_io_handle_t io;
    esp_lcd_panel_io_spi_config_t io_cfg = {};
    io_cfg.dc_gpio_num = LCD_DC;
    io_cfg.cs_gpio_num = LCD_CS;
    io_cfg.pclk_hz = 40 * 1000 * 1000;
    io_cfg.lcd_cmd_bits = 8;
    io_cfg.lcd_param_bits = 8;
    io_cfg.trans_queue_depth = 10;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(SPI2_HOST, &io_cfg, &io));

    // --- ST7796 config
    esp_lcd_panel_dev_config_t panel_cfg = {};
    panel_cfg.reset_gpio_num = LCD_RST;
    panel_cfg.color_space = ESP_LCD_COLOR_SPACE_RGB;
    panel_cfg.bits_per_pixel = 16;

    ESP_ERROR_CHECK(esp_lcd_new_panel_st7796(io, &panel_cfg, &panel));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel));

    // Fix orientation
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel, true));
    //ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel, true, false));

    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel, true));



    // --- Backlight
    InitBacklight();

    ESP_LOGI(TAG, "Filling screen red for test...");
    static uint16_t red_buf[LCD_HRES * 10];
    for (int i = 0; i < LCD_HRES * 10; i++) {
        red_buf[i] = 0xF800;
    }
    for (int y = 0; y < LCD_VRES; y += 10) {
        esp_lcd_panel_draw_bitmap(panel, 0, y, LCD_HRES, y + 10, red_buf);
    }
    ESP_LOGI(TAG, "Red fill complete");
    vTaskDelay(pdMS_TO_TICKS(5000));


    // --- LVGL display setup ---
    size_t buf_pixels = LCD_HRES * 32;
    buf1 = (lv_color_t *)heap_caps_malloc(buf_pixels * sizeof(lv_color_t), MALLOC_CAP_DMA | MALLOC_CAP_SPIRAM);
    buf2 = (lv_color_t *)heap_caps_malloc(buf_pixels * sizeof(lv_color_t), MALLOC_CAP_DMA | MALLOC_CAP_SPIRAM);
    assert(buf1 && buf2);

    lv_disp_draw_buf_init(&drawBuf, buf1, buf2, buf_pixels);

    lv_disp_drv_init(&dispDrv);
    dispDrv.flush_cb = LvglFlushCb;
    dispDrv.draw_buf = &drawBuf;
    dispDrv.hor_res = LCD_HRES;
    dispDrv.ver_res = LCD_VRES;
    dispDrv.user_data = this;  // so we can access `panel`
    disp = lv_disp_drv_register(&dispDrv);

    ESP_LOGI(TAG, "WT32-SC01 LVGL display registered");
}

void Display_WT32SC01::LvglFlushCb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p)
{
    auto *self = static_cast<Display_WT32SC01 *>(drv->user_data);
    if (!self || !self->panel) {
        lv_disp_flush_ready(drv);
        return;
    }

    esp_err_t err = esp_lcd_panel_draw_bitmap(
        self->panel,
        area->x1, area->y1,
        area->x2 + 1, area->y2 + 1,
        color_p);

    if (err != ESP_OK)
        ESP_LOGE(TAG, "Flush failed: %s", esp_err_to_name(err));

    lv_disp_flush_ready(drv);
}


void Display_WT32SC01::InitBacklight()
{
    ledc_timer_config_t tcfg = {};
    tcfg.speed_mode = LEDC_LOW_SPEED_MODE;
    tcfg.timer_num = LEDC_TIMER_0;
    tcfg.duty_resolution = LEDC_TIMER_10_BIT;
    tcfg.freq_hz = 5000;
    tcfg.clk_cfg = LEDC_AUTO_CLK;
    ESP_ERROR_CHECK(ledc_timer_config(&tcfg));

    ledc_channel_config_t ccfg = {};
    ccfg.gpio_num = LCD_BL;
    ccfg.speed_mode = LEDC_LOW_SPEED_MODE;
    ccfg.channel = LEDC_CHANNEL_0;
    ccfg.timer_sel = LEDC_TIMER_0;
    ccfg.duty = 512;
    ccfg.hpoint = 0;
    ESP_ERROR_CHECK(ledc_channel_config(&ccfg));
}

void Display_WT32SC01::SetBrightness(uint8_t percent)
{
    if (percent > 100)
        percent = 100;
    uint32_t duty = (1 << 10) * percent / 100;
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}
