#include "Display_WT32SC01.h"
#include "esp_lcd_ili9488.h"
#include <cassert>

Display_WT32SC01::~Display_WT32SC01()
{
    if (lvTickTimer)
        esp_timer_stop(lvTickTimer);
    free(buf1);
    free(buf2);
}

void Display_WT32SC01::Init()
{
    ESP_LOGI(TAG, "Initializing WT32-SC01 display");

    // --- SPI bus config
    spi_bus_config_t buscfg = {};
    buscfg.sclk_io_num = LCD_CLK;
    buscfg.mosi_io_num = LCD_MOSI;
    buscfg.miso_io_num = -1;
    buscfg.max_transfer_sz = LCD_HRES * 40 * sizeof(lv_color_t);
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));

    // --- ILI9488 panel IO
    esp_lcd_panel_io_handle_t io;
    esp_lcd_panel_io_spi_config_t io_cfg = {};
    io_cfg.dc_gpio_num = LCD_DC;
    io_cfg.cs_gpio_num = LCD_CS;
    io_cfg.pclk_hz = 40 * 1000 * 1000;
    io_cfg.lcd_cmd_bits = 8;
    io_cfg.lcd_param_bits = 8;
    io_cfg.trans_queue_depth = 10;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(SPI2_HOST, &io_cfg, &io));

    // --- Panel config
    esp_lcd_panel_dev_config_t panel_cfg = {};
    panel_cfg.reset_gpio_num = LCD_RST;
    panel_cfg.color_space = ESP_LCD_COLOR_SPACE_RGB;
    panel_cfg.bits_per_pixel = 16;

    ESP_ERROR_CHECK(esp_lcd_new_panel_ili9488(io, &panel_cfg, 16, &panel));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel));


    // --- Backlight
    InitBacklight();

    // --- LVGL init
    lv_init();

    size_t buf_pixels = LCD_HRES * 32;
    buf1 = (lv_color_t *)heap_caps_malloc(buf_pixels * sizeof(lv_color_t), MALLOC_CAP_DMA | MALLOC_CAP_SPIRAM);
    buf2 = (lv_color_t *)heap_caps_malloc(buf_pixels * sizeof(lv_color_t), MALLOC_CAP_DMA | MALLOC_CAP_SPIRAM);

    assert(buf1 && buf2);
    lv_disp_draw_buf_init(&drawBuf, buf1, buf2, buf_pixels);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.flush_cb = LvglFlushCb;
    disp_drv.draw_buf = &drawBuf;
    disp_drv.hor_res = LCD_HRES;
    disp_drv.ver_res = LCD_VRES;
    disp_drv.user_data = this;
    lv_disp_drv_register(&disp_drv);

    // --- LVGL tick
    esp_timer_create_args_t tconf = {};
    tconf.callback = &LvglTickCb;
    tconf.arg = this;
    tconf.name = "lv_tick";
    ESP_ERROR_CHECK(esp_timer_create(&tconf, &lvTickTimer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvTickTimer, 5 * 1000)); // 5 ms

    // --- Test text
    lv_obj_t *label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "Hello WT32-SC01 👋");
    lv_obj_center(label);

    ESP_LOGI(TAG, "Display init complete");
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

void Display_WT32SC01::LvglFlushCb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p)
{
    auto *self = static_cast<Display_WT32SC01 *>(drv->user_data);
    if (self && self->panel)
        esp_lcd_panel_draw_bitmap(self->panel, area->x1, area->y1, area->x2 + 1, area->y2 + 1, color_p);
    lv_disp_flush_ready(drv);
}

void Display_WT32SC01::LvglTickCb(void *arg)
{
    (void)arg;
    lv_tick_inc(5);
}

void Display_WT32SC01::Tick()
{
    lv_timer_handler();
}

void Display_WT32SC01::Update()
{
    // Future: handle LVGL tasks or custom drawing
}
