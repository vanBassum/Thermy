#include "DisplayManager.h"
#include <algorithm>
#include <cinttypes>
#include "DateTime.h"

static const lv_color_t channelColors[4] = {
    lv_palette_main(LV_PALETTE_RED),
    lv_palette_main(LV_PALETTE_BLUE),
    lv_palette_main(LV_PALETTE_GREEN),
    lv_palette_main(LV_PALETTE_YELLOW)
};

DisplayManager::DisplayManager(ServiceProvider &ctx)
    : networkManager(ctx.getNetworkManager())
    , sensorManager(ctx.getSensorManager())
{
}

void DisplayManager::Init()
{
    auto init = initState.TryBeginInit();
    if (!init)
        return;

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

    init.SetReady();
    ESP_LOGI(TAG, "DisplayManager initialized successfully.");
}

void DisplayManager::Work()
{
    UiSetup();

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

        vTaskDelay(pdMS_TO_TICKS(std::clamp(delayMs, (uint32_t)5, (uint32_t)100)));
    }
}

void DisplayManager::LvglTickCb(void *arg)
{
    (void)arg;
    lv_tick_inc(5);
}

// ── Main UI ──────────────────────────────────────────────────

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
    lv_obj_set_style_text_font(labelTime, &lv_font_montserrat_20, LV_PART_MAIN);

    labelIP = lv_label_create(lv_scr_act());
    lv_obj_align(labelIP, LV_ALIGN_TOP_RIGHT, -15, 8);
    lv_label_set_text(labelIP, "No IP");
    lv_obj_set_style_text_color(labelIP, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(labelIP, &lv_font_montserrat_20, LV_PART_MAIN);

    // --- Temperature boxes ---
    static const lv_coord_t startY = 40;
    static const lv_coord_t startX = 10;
    static const lv_coord_t spacing = 15;
    static const lv_coord_t boxW = 100;
    static const lv_coord_t boxH = 55;

    for (int i = 0; i < 4; i++)
    {
        lv_obj_t *box = lv_obj_create(lv_scr_act());
        lv_obj_set_size(box, boxW, boxH);
        lv_obj_set_pos(box, startX, startY + i * (boxH + spacing));

        lv_obj_set_style_bg_color(box, lv_color_hex(0x202020), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(box, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_width(box, 3, LV_PART_MAIN);
        lv_obj_set_style_border_color(box, channelColors[i], LV_PART_MAIN);
        lv_obj_set_style_radius(box, 8, LV_PART_MAIN);
        lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *label = lv_label_create(box);
        lv_label_set_text(label, "--.--");
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_text_color(label, channelColors[i], LV_PART_MAIN);
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

    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    lv_chart_set_update_mode(chart, LV_CHART_UPDATE_MODE_SHIFT);
    lv_chart_set_point_count(chart, 120);
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
    lv_obj_set_style_size(chart, 0, LV_PART_INDICATOR);
    lv_chart_set_div_line_count(chart, 5, 5);

    lv_obj_set_style_bg_color(chart, lv_color_hex(0x101010), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(chart, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(chart, 2, LV_PART_MAIN);
    lv_obj_set_style_border_color(chart, lv_palette_main(LV_PALETTE_GREY), LV_PART_MAIN);
    lv_obj_set_style_radius(chart, 8, LV_PART_MAIN);

    for (int i = 0; i < 4; i++)
        chartSeries[i] = lv_chart_add_series(chart, channelColors[i], LV_CHART_AXIS_PRIMARY_Y);
}

void DisplayManager::UiUpdate()
{
    char buf[32];

    // Update time
    DateTime now = DateTime::Now();
    now.ToStringLocal(buf, sizeof(buf), "%H:%M:%S");
    lv_label_set_text(labelTime, buf);

    // Update IP
    auto status = networkManager.wifi().getStatus();
    if (status.has_ipv4)
        snprintf(buf, sizeof(buf), IPSTR, IP2STR(&status.ipv4.ip));
    else
        snprintf(buf, sizeof(buf), "No IP");
    lv_label_set_text(labelIP, buf);

    // Update chart counter
    chartCounter++;
    if (chartCounter > 60)
        chartCounter = 0;

    // Update temperature slots
    for (int i = 0; i < 4; i++)
    {
        if (sensorManager.IsSlotActive(i))
        {
            float temp = sensorManager.GetTemperature(i);
            snprintf(buf, sizeof(buf), "%.2f", temp);
            lv_label_set_text(tempLabels[i], buf);

            if (chartCounter == 0)
                lv_chart_set_next_value(chart, chartSeries[i], (lv_coord_t)temp);
        }
        else
        {
            lv_label_set_text(tempLabels[i], "--.--");
        }
    }

    lv_chart_refresh(chart);

    // Check for pending sensors (only when no popup is showing)
    if (!assignPopup && sensorManager.HasPendingSensor())
    {
        ShowAssignPopup(sensorManager.GetPendingSensorAddress());
    }

    // Auto-assign timeout
    if (assignPopup && (xTaskGetTickCount() - popupShownAt > POPUP_TIMEOUT))
    {
        ESP_LOGI(TAG, "Popup timeout, auto-assigning");
        AutoAssignToFirstEmpty();
    }
}

// ── Assignment popup ─────────────────────────────────────────

void DisplayManager::ShowAssignPopup(uint64_t address)
{
    if (assignPopup)
        return;

    popupSensorAddress = address;
    popupShownAt = xTaskGetTickCount();

    char body[64];
    snprintf(body, sizeof(body), "Sensor %016" PRIX64 "\nAssign to slot:", address);

    static const char *btnTexts[] = {"Red", "Blue", "Green", "Yellow", ""};
    assignPopup = lv_msgbox_create(NULL, "New Sensor", body, btnTexts, false);
    lv_obj_center(assignPopup);
    lv_obj_add_event_cb(assignPopup, PopupEventCb, LV_EVENT_VALUE_CHANGED, this);

    ESP_LOGI(TAG, "Showing assignment popup for %016" PRIX64, address);
}

void DisplayManager::CloseAssignPopup()
{
    if (assignPopup)
    {
        lv_msgbox_close(assignPopup);
        assignPopup = nullptr;
        popupSensorAddress = 0;
    }
}

void DisplayManager::OnSlotSelected(int slot)
{
    if (slot < 0 || slot >= 4)
        return;

    ESP_LOGI(TAG, "User selected slot %d for sensor %016" PRIX64, slot, popupSensorAddress);
    sensorManager.AssignPendingToSlot(slot);
    CloseAssignPopup();
}

void DisplayManager::AutoAssignToFirstEmpty()
{
    for (int i = 0; i < 4; i++)
    {
        if (sensorManager.GetSlotAddress(i) == 0)
        {
            OnSlotSelected(i);
            return;
        }
    }

    // No empty slot, dismiss the pending sensor
    ESP_LOGW(TAG, "No empty slot available, dismissing pending sensor");
    sensorManager.DismissPendingSensor();
    CloseAssignPopup();
}

void DisplayManager::PopupEventCb(lv_event_t *e)
{
    auto *self = static_cast<DisplayManager *>(lv_event_get_user_data(e));
    lv_obj_t *mbox = lv_event_get_current_target(e);
    uint16_t btn = lv_msgbox_get_active_btn(mbox);
    if (btn < 4)
    {
        self->OnSlotSelected(btn);
    }
}
