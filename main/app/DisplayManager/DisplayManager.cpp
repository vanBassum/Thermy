#include "DisplayManager.h"

DisplayManager::DisplayManager(ServiceProvider &ctx)
{
}

void DisplayManager::Init()
{
    if (initGuard.IsReady())
        return;

    LOCK(mutex);
    ESP_LOGI(TAG, "Initializing DisplayManager...");

    lv_init();            // LVGL first
    display.Init();       // This now sets up the LVGL driver too

    // --- LVGL tick (5ms)
    timer.Init("LvglTickTimer", pdMS_TO_TICKS(5), true);
    timer.SetHandler([this]() { LvglTickCb(this); });
    timer.Start();

    // --- Start LVGL task ---
    task.Init("DisplayTask", 5, 4096);
    task.SetHandler([this]() { Work(); });
    task.Run();

    // --- LVGL visual test ---
    lv_obj_clean(lv_scr_act());

    // Set screen background to solid black (prevents white AA edges)
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, LV_PART_MAIN);

    // Box + label helper lambda
    auto make_box = [](lv_color_t color, const char *text, int x, int y) {
        lv_obj_t *box = lv_obj_create(lv_scr_act());
        lv_obj_set_size(box, 100, 60);
        lv_obj_set_pos(box, x, y);
        lv_obj_set_style_bg_color(box, color, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(box, LV_OPA_COVER, LV_PART_MAIN);   // Fully opaque
        lv_obj_set_style_border_width(box, 0, LV_PART_MAIN);        // No border AA

        lv_obj_t *label = lv_label_create(box);
        lv_label_set_text(label, text);
        lv_obj_center(label);
    };

    // Draw solid boxes for color verification
    make_box(lv_color_hex(0xFF0000), "RED",   40, 40);
    make_box(lv_color_hex(0x00FF00), "GREEN", 160, 40);
    make_box(lv_color_hex(0x0000FF), "BLUE",  280, 40);
    make_box(lv_color_hex(0xFFFF00), "YELLOW", 400, 40);
    make_box(lv_color_hex(0xFFFFFF), "WHITE",  40, 120);
    make_box(lv_color_hex(0x000000), "BLACK", 160, 120);

    // Text to verify rendering
    lv_obj_t *label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "LVGL WT32-SC01 Test");
    lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, -20);

    // Force immediate render
    lv_refr_now(NULL);



    initGuard.SetReady();
    ESP_LOGI(TAG, "DisplayManager initialized successfully.");
}

void DisplayManager::Work()
{
    const TickType_t tickPeriod = pdMS_TO_TICKS(5);
    uint32_t delayMs;

    while (true)
    {
        // Process LVGL tasks and get how long to wait until the next one
        delayMs = lv_timer_handler();

        // Convert to ticks, with a safe minimum of 5ms
        if (delayMs < 5)
            delayMs = 5;

        vTaskDelay(pdMS_TO_TICKS(delayMs));
    }
}


void DisplayManager::LvglTickCb(void *arg)
{
    (void)arg;
    lv_tick_inc(5);
}
