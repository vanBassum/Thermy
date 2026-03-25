#include "DisplayManager.h"
#include <algorithm>
#include <cinttypes>
#include "DateTime.h"
#include "esp_system.h"

static const lv_color_t channelColors[4] = {
    lv_palette_main(LV_PALETTE_RED),
    lv_palette_main(LV_PALETTE_BLUE),
    lv_palette_main(LV_PALETTE_GREEN),
    lv_palette_main(LV_PALETTE_YELLOW)
};

DisplayManager::DisplayManager(ServiceProvider &ctx)
    : networkManager(ctx.getNetworkManager())
    , sensorManager(ctx.getSensorManager())
    , settingsManager(ctx.getSettingsManager())
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
    lv_obj_clear_flag(lv_scr_act(), LV_OBJ_FLAG_SCROLLABLE);

    // --- Top bar ---
    labelTime = lv_label_create(lv_scr_act());
    lv_obj_align(labelTime, LV_ALIGN_TOP_LEFT, 15, 8);
    lv_label_set_text(labelTime, "00:00:00");
    lv_obj_set_style_text_color(labelTime, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(labelTime, &lv_font_montserrat_20, LV_PART_MAIN);

    labelIP = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_color(labelIP, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(labelIP, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_label_set_text(labelIP, "No IP");
    lv_obj_align(labelIP, LV_ALIGN_TOP_RIGHT, -50, 8);

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

    // Gear button (created last so it's on top of all other objects)
    lv_obj_t *gearBtn = lv_btn_create(lv_scr_act());
    lv_obj_set_size(gearBtn, 40, 36);
    lv_obj_align(gearBtn, LV_ALIGN_TOP_RIGHT, -2, 0);
    lv_obj_set_style_bg_color(gearBtn, lv_color_hex(0x303030), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(gearBtn, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(gearBtn, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(gearBtn, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(gearBtn, 6, LV_PART_MAIN);

    lv_obj_t *gearIcon = lv_label_create(gearBtn);
    lv_label_set_text(gearIcon, LV_SYMBOL_SETTINGS);
    lv_obj_set_style_text_color(gearIcon, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(gearIcon, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_center(gearIcon);
    lv_obj_add_event_cb(gearBtn, GearBtnCb, LV_EVENT_CLICKED, this);
}

void DisplayManager::UiUpdate()
{
    // Don't update main UI while settings or popup is open
    if (settingsPanel)
        return;

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
        ESP_LOGD(TAG, "Popup timeout, auto-assigning");
        AutoAssignToFirstEmpty();
    }
}

// ── Assignment popup ─────────────────────────────────────────

static const char *slotNames[4] = {"Red", "Blue", "Green", "Yellow"};

void DisplayManager::ShowAssignPopup(uint64_t address)
{
    if (assignPopup)
        return;

    popupSensorAddress = address;
    popupShownAt = xTaskGetTickCount();

    // Dark overlay container (full screen)
    assignPopup = lv_obj_create(lv_scr_act());
    lv_obj_set_size(assignPopup, LCD_HRES, LCD_VRES);
    lv_obj_set_pos(assignPopup, 0, 0);
    lv_obj_set_style_bg_color(assignPopup, lv_color_hex(0x101010), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(assignPopup, LV_OPA_90, LV_PART_MAIN);
    lv_obj_set_style_border_width(assignPopup, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(assignPopup, 0, LV_PART_MAIN);
    lv_obj_clear_flag(assignPopup, LV_OBJ_FLAG_SCROLLABLE);

    // Title
    lv_obj_t *title = lv_label_create(assignPopup);
    lv_label_set_text(title, "New Sensor Detected");
    lv_obj_set_style_text_color(title, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

    // Address label
    char addrStr[32];
    snprintf(addrStr, sizeof(addrStr), "%016" PRIX64, address);
    lv_obj_t *addrLabel = lv_label_create(assignPopup);
    lv_label_set_text(addrLabel, addrStr);
    lv_obj_set_style_text_color(addrLabel, lv_palette_main(LV_PALETTE_GREY), LV_PART_MAIN);
    lv_obj_set_style_text_font(addrLabel, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(addrLabel, LV_ALIGN_TOP_MID, 0, 50);

    // Subtitle
    lv_obj_t *subtitle = lv_label_create(assignPopup);
    lv_label_set_text(subtitle, "Assign to slot:");
    lv_obj_set_style_text_color(subtitle, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(subtitle, LV_ALIGN_TOP_MID, 0, 75);

    // 4 big color buttons in a row
    static constexpr lv_coord_t btnW = 100;
    static constexpr lv_coord_t btnH = 60;
    static constexpr lv_coord_t btnGap = 12;
    lv_coord_t totalW = 4 * btnW + 3 * btnGap;
    lv_coord_t startX = (LCD_HRES - totalW) / 2;
    lv_coord_t btnY = 110;

    for (int i = 0; i < 4; i++)
    {
        lv_obj_t *btn = lv_btn_create(assignPopup);
        lv_obj_set_size(btn, btnW, btnH);
        lv_obj_set_pos(btn, startX + i * (btnW + btnGap), btnY);

        lv_obj_set_style_bg_color(btn, channelColors[i], LV_PART_MAIN);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_radius(btn, 10, LV_PART_MAIN);
        lv_obj_set_style_border_width(btn, 2, LV_PART_MAIN);
        lv_obj_set_style_border_color(btn, lv_color_white(), LV_PART_MAIN);
        lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);

        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text(label, slotNames[i]);
        lv_obj_set_style_text_color(label, lv_color_white(), LV_PART_MAIN);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_14, LV_PART_MAIN);
        lv_obj_center(label);

        lv_obj_set_user_data(btn, this);
        lv_obj_add_event_cb(btn, PopupEventCb, LV_EVENT_CLICKED, reinterpret_cast<void *>(i));
    }

    // Timeout hint
    lv_obj_t *hint = lv_label_create(assignPopup);
    lv_label_set_text(hint, "Auto-assigns in 30s");
    lv_obj_set_style_text_color(hint, lv_palette_main(LV_PALETTE_GREY), LV_PART_MAIN);
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -20);

    ESP_LOGD(TAG, "Showing assignment popup for %016" PRIX64, address);
}

void DisplayManager::CloseAssignPopup()
{
    if (assignPopup)
    {
        lv_obj_del(assignPopup);
        assignPopup = nullptr;
        popupSensorAddress = 0;
    }
}

void DisplayManager::OnSlotSelected(int slot)
{
    if (slot < 0 || slot >= 4)
        return;

    ESP_LOGD(TAG, "User selected slot %d for sensor %016" PRIX64, slot, popupSensorAddress);
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

    ESP_LOGD(TAG, "No empty slot available, dismissing pending sensor");
    sensorManager.DismissPendingSensor();
    CloseAssignPopup();
}

void DisplayManager::PopupEventCb(lv_event_t *e)
{
    int slot = reinterpret_cast<int>(lv_event_get_user_data(e));
    lv_obj_t *btn = lv_event_get_target(e);
    auto *self = static_cast<DisplayManager *>(lv_obj_get_user_data(btn));
    if (self && slot >= 0 && slot < 4)
    {
        self->OnSlotSelected(slot);
    }
}

// ── Settings screen ──────────────────────────────────────────

void DisplayManager::ShowSettings()
{
    if (settingsPanel)
        return;

    // Close any open popup first
    CloseAssignPopup();

    // Full-screen dark panel
    settingsPanel = lv_obj_create(lv_scr_act());
    lv_obj_set_size(settingsPanel, LCD_HRES, LCD_VRES);
    lv_obj_set_pos(settingsPanel, 0, 0);
    lv_obj_set_style_bg_color(settingsPanel, lv_color_hex(0x101010), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(settingsPanel, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(settingsPanel, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(settingsPanel, 0, LV_PART_MAIN);
    lv_obj_clear_flag(settingsPanel, LV_OBJ_FLAG_SCROLLABLE);

    // --- Top bar: Back button + title ---
    lv_obj_t *backBtn = lv_btn_create(settingsPanel);
    lv_obj_set_size(backBtn, 60, 36);
    lv_obj_set_pos(backBtn, 10, 8);
    lv_obj_set_style_bg_color(backBtn, lv_color_hex(0x303030), LV_PART_MAIN);
    lv_obj_set_style_shadow_width(backBtn, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(backBtn, 6, LV_PART_MAIN);

    lv_obj_t *backLabel = lv_label_create(backBtn);
    lv_label_set_text(backLabel, LV_SYMBOL_LEFT " Back");
    lv_obj_set_style_text_color(backLabel, lv_color_white(), LV_PART_MAIN);
    lv_obj_center(backLabel);
    lv_obj_add_event_cb(backBtn, BackBtnCb, LV_EVENT_CLICKED, this);

    lv_obj_t *title = lv_label_create(settingsPanel);
    lv_label_set_text(title, LV_SYMBOL_SETTINGS " Settings");
    lv_obj_set_style_text_color(title, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 12);

    // --- WiFi SSID ---
    lv_obj_t *ssidLabel = lv_label_create(settingsPanel);
    lv_label_set_text(ssidLabel, LV_SYMBOL_WIFI " SSID");
    lv_obj_set_style_text_color(ssidLabel, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_pos(ssidLabel, 15, 55);

    ssidTextarea = lv_textarea_create(settingsPanel);
    lv_obj_set_size(ssidTextarea, 300, 36);
    lv_obj_set_pos(ssidTextarea, 120, 50);
    lv_textarea_set_one_line(ssidTextarea, true);
    lv_textarea_set_max_length(ssidTextarea, 32);
    lv_obj_set_style_bg_color(ssidTextarea, lv_color_hex(0x252525), LV_PART_MAIN);
    lv_obj_set_style_text_color(ssidTextarea, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_border_color(ssidTextarea, lv_palette_main(LV_PALETTE_GREY), LV_PART_MAIN);

    // Load current SSID
    char ssidBuf[33] = {};
    settingsManager.getString("wifi.ssid", ssidBuf, sizeof(ssidBuf));
    lv_textarea_set_text(ssidTextarea, ssidBuf);
    lv_obj_add_event_cb(ssidTextarea, TextareaFocusCb, LV_EVENT_FOCUSED, this);

    // --- WiFi Password ---
    lv_obj_t *passLabel = lv_label_create(settingsPanel);
    lv_label_set_text(passLabel, LV_SYMBOL_EYE_CLOSE " Pass");
    lv_obj_set_style_text_color(passLabel, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_pos(passLabel, 15, 97);

    passwordTextarea = lv_textarea_create(settingsPanel);
    lv_obj_set_size(passwordTextarea, 300, 36);
    lv_obj_set_pos(passwordTextarea, 120, 92);
    lv_textarea_set_one_line(passwordTextarea, true);
    lv_textarea_set_max_length(passwordTextarea, 64);
    lv_textarea_set_password_mode(passwordTextarea, true);
    lv_obj_set_style_bg_color(passwordTextarea, lv_color_hex(0x252525), LV_PART_MAIN);
    lv_obj_set_style_text_color(passwordTextarea, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_border_color(passwordTextarea, lv_palette_main(LV_PALETTE_GREY), LV_PART_MAIN);

    // Load current password
    char passBuf[65] = {};
    settingsManager.getString("wifi.password", passBuf, sizeof(passBuf));
    lv_textarea_set_text(passwordTextarea, passBuf);
    lv_obj_add_event_cb(passwordTextarea, TextareaFocusCb, LV_EVENT_FOCUSED, this);

    // --- Buttons ---
    // Clear Sensors
    lv_obj_t *clearBtn = lv_btn_create(settingsPanel);
    lv_obj_set_size(clearBtn, 150, 40);
    lv_obj_align(clearBtn, LV_ALIGN_TOP_LEFT, 15, 140);
    lv_obj_set_style_bg_color(clearBtn, lv_palette_main(LV_PALETTE_DEEP_ORANGE), LV_PART_MAIN);
    lv_obj_set_style_shadow_width(clearBtn, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(clearBtn, 6, LV_PART_MAIN);

    lv_obj_t *clearLabel = lv_label_create(clearBtn);
    lv_label_set_text(clearLabel, LV_SYMBOL_TRASH " Clear Sensors");
    lv_obj_set_style_text_color(clearLabel, lv_color_white(), LV_PART_MAIN);
    lv_obj_center(clearLabel);
    lv_obj_add_event_cb(clearBtn, ClearSensorsBtnCb, LV_EVENT_CLICKED, this);

    // Save & Reboot
    lv_obj_t *saveBtn = lv_btn_create(settingsPanel);
    lv_obj_set_size(saveBtn, 150, 40);
    lv_obj_align(saveBtn, LV_ALIGN_TOP_RIGHT, -15, 140);
    lv_obj_set_style_bg_color(saveBtn, lv_palette_main(LV_PALETTE_BLUE), LV_PART_MAIN);
    lv_obj_set_style_shadow_width(saveBtn, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(saveBtn, 6, LV_PART_MAIN);

    lv_obj_t *saveLabel = lv_label_create(saveBtn);
    lv_label_set_text(saveLabel, LV_SYMBOL_OK " Save & Reboot");
    lv_obj_set_style_text_color(saveLabel, lv_color_white(), LV_PART_MAIN);
    lv_obj_center(saveLabel);
    lv_obj_add_event_cb(saveBtn, SaveRebootBtnCb, LV_EVENT_CLICKED, this);

    // --- Keyboard (hidden until textarea focused) ---
    keyboard = lv_keyboard_create(settingsPanel);
    lv_obj_set_size(keyboard, LCD_HRES, 130);
    lv_obj_align(keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(keyboard, lv_color_hex(0x1a1a1a), LV_PART_MAIN);
    lv_obj_set_style_text_color(keyboard, lv_color_white(), LV_PART_ITEMS);
    lv_obj_set_style_bg_color(keyboard, lv_color_hex(0x333333), LV_PART_ITEMS);
    lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
}

void DisplayManager::CloseSettings()
{
    if (settingsPanel)
    {
        keyboard = nullptr;
        ssidTextarea = nullptr;
        passwordTextarea = nullptr;
        lv_obj_del(settingsPanel);
        settingsPanel = nullptr;
    }
}

void DisplayManager::GearBtnCb(lv_event_t *e)
{
    auto *self = static_cast<DisplayManager *>(lv_event_get_user_data(e));
    self->ShowSettings();
}

void DisplayManager::BackBtnCb(lv_event_t *e)
{
    auto *self = static_cast<DisplayManager *>(lv_event_get_user_data(e));
    self->CloseSettings();
}

void DisplayManager::SaveRebootBtnCb(lv_event_t *e)
{
    auto *self = static_cast<DisplayManager *>(lv_event_get_user_data(e));

    const char *ssid = lv_textarea_get_text(self->ssidTextarea);
    const char *pass = lv_textarea_get_text(self->passwordTextarea);

    self->settingsManager.setString("wifi.ssid", ssid);
    self->settingsManager.setString("wifi.password", pass);
    self->settingsManager.Save();

    ESP_LOGI(TAG, "WiFi settings saved, rebooting...");
    vTaskDelay(pdMS_TO_TICKS(500));
    esp_restart();
}

void DisplayManager::ClearSensorsBtnCb(lv_event_t *e)
{
    auto *self = static_cast<DisplayManager *>(lv_event_get_user_data(e));
    self->sensorManager.ClearAllSlots();
    self->CloseSettings();
}

void DisplayManager::TextareaFocusCb(lv_event_t *e)
{
    auto *self = static_cast<DisplayManager *>(lv_event_get_user_data(e));
    lv_obj_t *ta = lv_event_get_target(e);

    if (self->keyboard)
    {
        lv_keyboard_set_textarea(self->keyboard, ta);
        lv_obj_clear_flag(self->keyboard, LV_OBJ_FLAG_HIDDEN);
    }
}
