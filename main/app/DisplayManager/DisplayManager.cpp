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

// Black background for contrast
lv_obj_set_style_bg_color(lv_scr_act(), lv_color_make(0, 0, 0), 0);

// === RED BOX ===
lv_obj_t *red_box = lv_obj_create(lv_scr_act());
lv_obj_set_size(red_box, 80, 80);
lv_obj_set_style_bg_color(red_box, lv_color_make(255, 0, 0), 0);
lv_obj_align(red_box, LV_ALIGN_TOP_LEFT, 10, 10);

lv_obj_t *red_label = lv_label_create(lv_scr_act());
lv_label_set_text(red_label, "RED");
lv_obj_set_style_text_color(red_label, lv_color_make(255, 255, 255), 0);
lv_obj_align_to(red_label, red_box, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);

// === GREEN BOX ===
lv_obj_t *green_box = lv_obj_create(lv_scr_act());
lv_obj_set_size(green_box, 80, 80);
lv_obj_set_style_bg_color(green_box, lv_color_make(0, 255, 0), 0);
lv_obj_align(green_box, LV_ALIGN_TOP_MID, 0, 10);

lv_obj_t *green_label = lv_label_create(lv_scr_act());
lv_label_set_text(green_label, "GREEN");
lv_obj_set_style_text_color(green_label, lv_color_make(255, 255, 255), 0);
lv_obj_align_to(green_label, green_box, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);

// === BLUE BOX ===
lv_obj_t *blue_box = lv_obj_create(lv_scr_act());
lv_obj_set_size(blue_box, 80, 80);
lv_obj_set_style_bg_color(blue_box, lv_color_make(0, 0, 255), 0);
lv_obj_align(blue_box, LV_ALIGN_TOP_RIGHT, -10, 10);

lv_obj_t *blue_label = lv_label_create(lv_scr_act());
lv_label_set_text(blue_label, "BLUE");
lv_obj_set_style_text_color(blue_label, lv_color_make(255, 255, 255), 0);
lv_obj_align_to(blue_label, blue_box, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);

// === GRADIENT TEST ===
lv_obj_t *gradient = lv_obj_create(lv_scr_act());
lv_obj_set_size(gradient, 200, 40);
lv_obj_set_style_bg_color(gradient, lv_color_make(255, 0, 0), 0);
lv_obj_set_style_bg_grad_color(gradient, lv_color_make(0, 0, 255), 0);
lv_obj_set_style_bg_grad_dir(gradient, LV_GRAD_DIR_HOR, 0);
lv_obj_align(gradient, LV_ALIGN_CENTER, 0, -30);

lv_obj_t *grad_label = lv_label_create(lv_scr_act());
lv_label_set_text(grad_label, "Gradient: Red → Blue");
lv_obj_set_style_text_color(grad_label, lv_color_make(255, 255, 255), 0);
lv_obj_align_to(grad_label, gradient, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);

// === WHITE LABEL CENTER ===
lv_obj_t *hello = lv_label_create(lv_scr_act());
lv_label_set_text(hello, "Hello LVGL!");
lv_obj_set_style_text_color(hello, lv_color_make(255, 255, 255), 0);
lv_obj_align(hello, LV_ALIGN_BOTTOM_MID, 0, -30);

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
