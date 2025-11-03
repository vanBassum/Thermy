#include "DisplayManager.h"
#include "DateTime.h"
#include "core_utils.h"

DisplayManager::DisplayManager(ServiceProvider &ctx)
    : wifiManager(ctx.GetWifiManager())
{
}

void DisplayManager::Init()
{
    if (initGuard.IsReady())
        return;

    LOCK(mutex);
    ESP_LOGI(TAG, "Initializing DisplayManager...");

    lv_init();
    display.Init();

    // --- LVGL tick (5ms)
    timer.Init("LvglTickTimer", pdMS_TO_TICKS(5), true);
    timer.SetHandler([this]()
                     { LvglTickCb(this); });
    timer.Start();

    // --- Start LVGL task ---
    task.Init("DisplayTask", 5, 4096);
    task.SetHandler([this]()
                    { Work(); });
    task.Run();

    UiSetup();

    initGuard.SetReady();
    ESP_LOGI(TAG, "DisplayManager initialized successfully.");
}

void DisplayManager::Work()
{
    uint32_t delayMs;
    TickType_t lastUiUpdate = xTaskGetTickCount();

    while (true)
    {
        delayMs = lv_timer_handler();

        if (xTaskGetTickCount() - lastUiUpdate > pdMS_TO_TICKS(1000))
        {
            lastUiUpdate = xTaskGetTickCount();
            UiUpdate();
        }

        vTaskDelay(pdMS_TO_TICKS(CLAMP(delayMs, 5, 100)));
    }
}

void DisplayManager::LvglTickCb(void *arg)
{
    (void)arg;
    lv_tick_inc(5);
}

void DisplayManager::UiSetup()
{
    lv_obj_clean(lv_scr_act());
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, LV_PART_MAIN);

    // --- Top bar ---
    labelTime = lv_label_create(lv_scr_act());
    lv_obj_align(labelTime, LV_ALIGN_TOP_LEFT, 15, 8);
    lv_label_set_text(labelTime, "00:00:00");
    lv_obj_set_style_text_color(labelTime, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(labelTime, &lv_font_montserrat_14, LV_PART_MAIN);

    labelIP = lv_label_create(lv_scr_act());
    lv_obj_align(labelIP, LV_ALIGN_TOP_RIGHT, -15, 8);
    lv_label_set_text(labelIP, "192.168.0.123");
    lv_obj_set_style_text_color(labelIP, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(labelIP, &lv_font_montserrat_14, LV_PART_MAIN);

    // --- Temperature section ---
    static const lv_coord_t startY = 40;
    static const lv_coord_t startX = 10;
    static const lv_coord_t spacing = 15;
    static const lv_coord_t boxW = 100;
    static const lv_coord_t boxH = 50;

    // Define matching colors for each channel
    static const lv_color_t channelColors[4] = {
        lv_palette_main(LV_PALETTE_RED),
        lv_palette_main(LV_PALETTE_BLUE),
        lv_palette_main(LV_PALETTE_GREEN),
        lv_palette_main(LV_PALETTE_YELLOW)
    };

    for (int i = 0; i < 4; i++)
    {
        lv_obj_t *box = lv_obj_create(lv_scr_act());
        lv_obj_set_size(box, boxW, boxH);
        lv_obj_set_pos(box, startX, startY + i * (boxH + spacing));

        // Slightly dark background
        lv_obj_set_style_bg_color(box, lv_color_hex(0x202020), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(box, LV_OPA_COVER, LV_PART_MAIN);

        // Use chart color as border color
        lv_obj_set_style_border_width(box, 3, LV_PART_MAIN);
        lv_obj_set_style_border_color(box, channelColors[i], LV_PART_MAIN);

        lv_obj_set_style_radius(box, 8, LV_PART_MAIN);
        lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);

        // Temperature label
        lv_obj_t *label = lv_label_create(box);
        lv_label_set_text(label, "00.00");
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_text_color(label, channelColors[i], LV_PART_MAIN); // text matches chart
        lv_obj_set_style_text_font(label, &lv_font_montserrat_28, LV_PART_MAIN);

        tempBoxes[i] = box;
        tempLabels[i] = label;
    }


    // --- Temperature history chart ---
    const lv_coord_t chartX = startX + boxW + 20;
    const lv_coord_t chartY = startY;
    const lv_coord_t chartW = LCD_HRES - chartX - 10;
    const lv_coord_t chartH = (boxH + spacing) * 4 - spacing;

    chart = lv_chart_create(lv_scr_act());
    lv_obj_set_size(chart, chartW, chartH);
    lv_obj_set_pos(chart, chartX, chartY);

    // Line chart without points
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    lv_chart_set_update_mode(chart, LV_CHART_UPDATE_MODE_SHIFT);
    lv_chart_set_point_count(chart, 30);
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0, 100);

    // Hide dots, show only lines
    lv_obj_set_style_size(chart, 0, LV_PART_INDICATOR); // remove point markers

    // --- Enable Y-axis ticks and labels ---
    // lv_chart_set_axis_tick(chart, LV_CHART_AXIS_PRIMARY_Y, 10, 5, 5, 2, true, 50);

    // Chart appearance
    lv_obj_set_style_bg_color(chart, lv_color_hex(0x101010), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(chart, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(chart, 2, LV_PART_MAIN);
    lv_obj_set_style_border_color(chart, lv_palette_main(LV_PALETTE_GREY), LV_PART_MAIN);
    lv_obj_set_style_radius(chart, 8, LV_PART_MAIN);

    // Create one series per temperature channel
    chartSeries[0] = lv_chart_add_series(chart, channelColors[0] , LV_CHART_AXIS_PRIMARY_Y);
    chartSeries[1] = lv_chart_add_series(chart, channelColors[1] , LV_CHART_AXIS_PRIMARY_Y);
    chartSeries[2] = lv_chart_add_series(chart, channelColors[2] , LV_CHART_AXIS_PRIMARY_Y);
    chartSeries[3] = lv_chart_add_series(chart, channelColors[3] , LV_CHART_AXIS_PRIMARY_Y);
}

void DisplayManager::UiUpdate()
{
    // Update time
    DateTime now = DateTime::Now();
    char buf[32];

    // day-month hh mm ss
    now.ToStringLocal(buf, sizeof(buf), "%d-%m %H:%M:%S");
    lv_label_set_text(labelTime, buf);

    // Dummy IP placeholder (replace with actual later)
    if (!wifiManager.GetIp(buf, sizeof(buf)))
        snprintf(buf, sizeof(buf), "No IP");
    lv_label_set_text(labelIP, buf);

    // Update dummy temperatures
    for (int i = 0; i < 4; i++)
    {
        float temp = 20.0f + (rand() % 100) / 10.0f; // 20.0–29.9°C
        snprintf(buf, sizeof(buf), "%.2f", temp);
        lv_label_set_text(tempLabels[i], buf);

        // Add temperature to chart
        lv_chart_set_next_value(chart, chartSeries[i], (lv_coord_t)temp);
    }

    // Refresh chart to reflect new data
    lv_chart_refresh(chart);
}
