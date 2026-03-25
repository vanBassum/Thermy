#include "DisplaySettings.h"
#include "SettingsManager/SettingsManager.h"
#include "SensorManager/SensorManager.h"
#include "NetworkManager/NetworkManager.h"
#include "esp_log.h"
#include "esp_system.h"
#include <cstdio>

DisplaySettings::DisplaySettings(SettingsManager &settings, SensorManager &sensors, NetworkManager &network)
    : settingsManager(settings), sensorManager(sensors), networkManager(network)
{
}

// ── Helpers ──────────────────────────────────────────────────

lv_obj_t *DisplaySettings::CreatePanel(lv_obj_t *parent)
{
    lv_obj_t *p = lv_obj_create(parent);
    lv_obj_set_size(p, LCD_HRES, LCD_VRES);
    lv_obj_set_pos(p, 0, 0);
    lv_obj_set_style_bg_color(p, lv_color_hex(0x101010), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(p, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(p, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(p, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(p, 0, LV_PART_MAIN);
    lv_obj_clear_flag(p, LV_OBJ_FLAG_SCROLLABLE);
    return p;
}

lv_obj_t *DisplaySettings::CreateTopBar(lv_obj_t *parent, const char *title, lv_event_cb_t backCb)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 70, 32);
    lv_obj_set_pos(btn, 8, 6);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x303030), LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(btn, 6, LV_PART_MAIN);

    lv_obj_t *backLabel = lv_label_create(btn);
    lv_label_set_text(backLabel, LV_SYMBOL_LEFT " Back");
    lv_obj_set_style_text_color(backLabel, lv_color_white(), LV_PART_MAIN);
    lv_obj_center(backLabel);
    lv_obj_add_event_cb(btn, backCb, LV_EVENT_CLICKED, this);

    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, title);
    lv_obj_set_style_text_color(lbl, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, 10);

    return btn;
}

lv_obj_t *DisplaySettings::CreateButton(lv_obj_t *parent, const char *text, lv_color_t color,
                                         lv_coord_t w, lv_coord_t h, lv_event_cb_t cb)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, w, h);
    lv_obj_set_style_bg_color(btn, color, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(btn, 8, LV_PART_MAIN);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_color(lbl, lv_color_white(), LV_PART_MAIN);
    lv_obj_center(lbl);

    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, this);
    return btn;
}

lv_obj_t *DisplaySettings::CreateTextRow(lv_obj_t *parent, const char *label, const char *value,
                                          lv_coord_t y, int maxLen, bool password)
{
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, label);
    lv_obj_set_style_text_color(lbl, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_pos(lbl, 12, y + 8);

    lv_obj_t *ta = lv_textarea_create(parent);
    lv_obj_set_size(ta, 280, 34);
    lv_obj_set_pos(ta, 120, y);
    lv_textarea_set_one_line(ta, true);
    lv_textarea_set_max_length(ta, maxLen);
    lv_textarea_set_text(ta, value);
    if (password)
        lv_textarea_set_password_mode(ta, true);

    lv_obj_set_style_bg_color(ta, lv_color_hex(0x252525), LV_PART_MAIN);
    lv_obj_set_style_text_color(ta, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_border_color(ta, lv_color_hex(0x444444), LV_PART_MAIN);
    lv_obj_add_event_cb(ta, TextareaFocusCb, LV_EVENT_FOCUSED, this);

    return ta;
}

void DisplaySettings::ShowKeyboard(lv_obj_t *textarea)
{
    if (!keyboard)
    {
        keyboard = lv_keyboard_create(panel);
        lv_obj_set_size(keyboard, LCD_HRES, 130);
        lv_obj_align(keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_obj_set_style_bg_color(keyboard, lv_color_hex(0x1a1a1a), LV_PART_MAIN);
        lv_obj_set_style_text_color(keyboard, lv_color_white(), LV_PART_ITEMS);
        lv_obj_set_style_bg_color(keyboard, lv_color_hex(0x333333), LV_PART_ITEMS);
    }
    lv_keyboard_set_textarea(keyboard, textarea);
    lv_obj_clear_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
}

void DisplaySettings::HideKeyboard()
{
    if (keyboard)
    {
        lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
    }
}

void DisplaySettings::ClearContent()
{
    if (contentArea)
    {
        lv_obj_del(contentArea);
        contentArea = nullptr;
    }
    keyboard = nullptr;
    HideKeyboard();
}

// ── Show / Close ─────────────────────────────────────────────

void DisplaySettings::Show(lv_obj_t *parent)
{
    if (panel)
        return;

    panel = CreatePanel(parent);
    ShowMenu();
}

void DisplaySettings::Close()
{
    if (panel)
    {
        contentArea = nullptr;
        keyboard = nullptr;
        lv_obj_del(panel);
        panel = nullptr;
    }
}

// ── Menu page ────────────────────────────────────────────────

void DisplaySettings::ShowMenu()
{
    ClearContent();

    contentArea = lv_obj_create(panel);
    lv_obj_set_size(contentArea, LCD_HRES, LCD_VRES);
    lv_obj_set_pos(contentArea, 0, 0);
    lv_obj_set_style_bg_opa(contentArea, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(contentArea, 0, LV_PART_MAIN);
    lv_obj_clear_flag(contentArea, LV_OBJ_FLAG_SCROLLABLE);

    CreateTopBar(contentArea, LV_SYMBOL_SETTINGS " Settings", CloseCb);

    // Category buttons - 3 big tiles
    static constexpr lv_coord_t btnW = 140;
    static constexpr lv_coord_t btnH = 100;
    static constexpr lv_coord_t gap = 15;
    lv_coord_t totalW = 3 * btnW + 2 * gap;
    lv_coord_t startX = (LCD_HRES - totalW) / 2;
    lv_coord_t y = 70;

    lv_obj_t *wifiBtn = CreateButton(contentArea, LV_SYMBOL_WIFI "\nWiFi",
                                      lv_palette_main(LV_PALETTE_BLUE), btnW, btnH, WifiBtnCb);
    lv_obj_set_pos(wifiBtn, startX, y);

    lv_obj_t *sensorBtn = CreateButton(contentArea, LV_SYMBOL_EYE_OPEN "\nSensors",
                                        lv_palette_main(LV_PALETTE_GREEN), btnW, btnH, SensorsBtnCb);
    lv_obj_set_pos(sensorBtn, startX + btnW + gap, y);

    lv_obj_t *systemBtn = CreateButton(contentArea, LV_SYMBOL_SETTINGS "\nSystem",
                                        lv_palette_main(LV_PALETTE_GREY), btnW, btnH, SystemBtnCb);
    lv_obj_set_pos(systemBtn, startX + 2 * (btnW + gap), y);

    // Reboot button at bottom
    lv_obj_t *rebootBtn = CreateButton(contentArea, LV_SYMBOL_POWER " Reboot",
                                        lv_palette_main(LV_PALETTE_RED), 140, 40, nullptr);
    lv_obj_align(rebootBtn, LV_ALIGN_BOTTOM_MID, 0, -15);
    lv_obj_add_event_cb(rebootBtn, [](lv_event_t *) {
        vTaskDelay(pdMS_TO_TICKS(200));
        esp_restart();
    }, LV_EVENT_CLICKED, nullptr);
}

// ── WiFi page ────────────────────────────────────────────────

void DisplaySettings::ShowWifiPage()
{
    ClearContent();

    contentArea = lv_obj_create(panel);
    lv_obj_set_size(contentArea, LCD_HRES, LCD_VRES);
    lv_obj_set_pos(contentArea, 0, 0);
    lv_obj_set_style_bg_opa(contentArea, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(contentArea, 0, LV_PART_MAIN);
    lv_obj_clear_flag(contentArea, LV_OBJ_FLAG_SCROLLABLE);

    CreateTopBar(contentArea, LV_SYMBOL_WIFI " WiFi", BackToMenuCb);

    // Scan button
    lv_obj_t *scanBtn = CreateButton(contentArea, LV_SYMBOL_REFRESH " Scan",
                                      lv_palette_main(LV_PALETTE_BLUE), 80, 30, ScanBtnCb);
    lv_obj_set_pos(scanBtn, LCD_HRES - 95, 6);

    // Network list (scrollable area)
    lv_obj_t *listArea = lv_obj_create(contentArea);
    lv_obj_set_size(listArea, LCD_HRES - 20, 130);
    lv_obj_set_pos(listArea, 10, 44);
    lv_obj_set_style_bg_color(listArea, lv_color_hex(0x1a1a1a), LV_PART_MAIN);
    lv_obj_set_style_border_color(listArea, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_border_width(listArea, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(listArea, 6, LV_PART_MAIN);
    lv_obj_set_style_pad_all(listArea, 4, LV_PART_MAIN);
    lv_obj_set_flex_flow(listArea, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(listArea, 2, LV_PART_MAIN);
    lv_obj_set_user_data(contentArea, listArea); // store for scan results

    // Scanning label
    lv_obj_t *scanLabel = lv_label_create(listArea);
    lv_label_set_text(scanLabel, "Tap 'Scan' to find networks");
    lv_obj_set_style_text_color(scanLabel, lv_palette_main(LV_PALETTE_GREY), LV_PART_MAIN);

    // Password field
    char passBuf[65] = {};
    settingsManager.getString("wifi.password", passBuf, sizeof(passBuf));
    lv_obj_t *passTa = CreateTextRow(contentArea, LV_SYMBOL_EYE_CLOSE " Pass", passBuf, 180, 64, true);
    lv_obj_set_user_data(contentArea->spec_attr->children[0], passTa); // bit hacky, store ta ref

    // Store password textarea for save callback
    lv_obj_set_user_data(panel, passTa);

    // Save & Reboot
    lv_obj_t *saveBtn = CreateButton(contentArea, LV_SYMBOL_OK " Save & Reboot",
                                      lv_palette_main(LV_PALETTE_BLUE), 180, 38, SaveWifiCb);
    lv_obj_set_pos(saveBtn, LCD_HRES - 195, 220);

    // Auto-scan on page open
    StartWifiScan();
}

void DisplaySettings::StartWifiScan()
{
    if (!contentArea)
        return;

    // Find the list area
    lv_obj_t *listArea = static_cast<lv_obj_t *>(lv_obj_get_user_data(contentArea));
    if (!listArea)
        return;

    lv_obj_clean(listArea);

    lv_obj_t *scanning = lv_label_create(listArea);
    lv_label_set_text(scanning, "Scanning...");
    lv_obj_set_style_text_color(scanning, lv_palette_main(LV_PALETTE_GREY), LV_PART_MAIN);

    // Force UI update
    lv_refr_now(NULL);

    // Scan
    WiFiInterface::ScanResult results[15] = {};
    int count = networkManager.wifi().Scan(results, 15);

    lv_obj_clean(listArea);

    if (count == 0)
    {
        lv_obj_t *noResults = lv_label_create(listArea);
        lv_label_set_text(noResults, "No networks found");
        lv_obj_set_style_text_color(noResults, lv_palette_main(LV_PALETTE_GREY), LV_PART_MAIN);
        return;
    }

    // Current SSID
    char currentSsid[33] = {};
    settingsManager.getString("wifi.ssid", currentSsid, sizeof(currentSsid));

    for (int i = 0; i < count; i++)
    {
        lv_obj_t *row = lv_btn_create(listArea);
        lv_obj_set_size(row, lv_pct(100), 28);
        lv_obj_set_style_radius(row, 4, LV_PART_MAIN);
        lv_obj_set_style_shadow_width(row, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_hor(row, 8, LV_PART_MAIN);

        bool isActive = strcmp(results[i].ssid, currentSsid) == 0;
        lv_obj_set_style_bg_color(row, isActive ? lv_palette_main(LV_PALETTE_BLUE) : lv_color_hex(0x2a2a2a), LV_PART_MAIN);

        // SSID + RSSI
        char rowText[48];
        const char *lock = results[i].secure ? LV_SYMBOL_EYE_CLOSE : "";
        snprintf(rowText, sizeof(rowText), "%s %s  (%ddBm)", lock, results[i].ssid, results[i].rssi);

        lv_obj_t *lbl = lv_label_create(row);
        lv_label_set_text(lbl, rowText);
        lv_obj_set_style_text_color(lbl, lv_color_white(), LV_PART_MAIN);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, LV_PART_MAIN);
        lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 0, 0);

        // Store SSID in user_data - need static storage
        // Use the label text itself as reference (LVGL copies it)
        lv_obj_add_event_cb(row, WifiNetworkCb, LV_EVENT_CLICKED, this);
    }
}

// ── Sensors page ─────────────────────────────────────────────

void DisplaySettings::ShowSensorsPage()
{
    ClearContent();

    contentArea = lv_obj_create(panel);
    lv_obj_set_size(contentArea, LCD_HRES, LCD_VRES);
    lv_obj_set_pos(contentArea, 0, 0);
    lv_obj_set_style_bg_opa(contentArea, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(contentArea, 0, LV_PART_MAIN);
    lv_obj_clear_flag(contentArea, LV_OBJ_FLAG_SCROLLABLE);

    CreateTopBar(contentArea, LV_SYMBOL_EYE_OPEN " Sensors", BackToMenuCb);

    // Graph range
    char buf[16];
    snprintf(buf, sizeof(buf), "%ld", settingsManager.getInt("graph.min", 0));
    lv_obj_t *minTa = CreateTextRow(contentArea, "Graph Min", buf, 50, 6);

    snprintf(buf, sizeof(buf), "%ld", settingsManager.getInt("graph.max", 100));
    lv_obj_t *maxTa = CreateTextRow(contentArea, "Graph Max", buf, 90, 6);

    // Scan interval
    snprintf(buf, sizeof(buf), "%ld", settingsManager.getInt("sensor.scan", 5000));
    CreateTextRow(contentArea, "Scan (ms)", buf, 130, 8);

    snprintf(buf, sizeof(buf), "%ld", settingsManager.getInt("sensor.read", 1000));
    CreateTextRow(contentArea, "Read (ms)", buf, 170, 8);

    // Clear sensors
    lv_obj_t *clearBtn = CreateButton(contentArea, LV_SYMBOL_TRASH " Clear All Sensors",
                                       lv_palette_main(LV_PALETTE_DEEP_ORANGE), 200, 40, ClearSensorsCb);
    lv_obj_align(clearBtn, LV_ALIGN_BOTTOM_LEFT, 15, -15);

    // Save
    lv_obj_t *saveBtn = CreateButton(contentArea, LV_SYMBOL_OK " Save & Reboot",
                                      lv_palette_main(LV_PALETTE_BLUE), 180, 40, SaveSystemCb);
    lv_obj_align(saveBtn, LV_ALIGN_BOTTOM_RIGHT, -15, -15);
}

// ── System page ──────────────────────────────────────────────

void DisplaySettings::ShowSystemPage()
{
    ClearContent();

    contentArea = lv_obj_create(panel);
    lv_obj_set_size(contentArea, LCD_HRES, LCD_VRES);
    lv_obj_set_pos(contentArea, 0, 0);
    lv_obj_set_style_bg_opa(contentArea, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(contentArea, 0, LV_PART_MAIN);
    lv_obj_clear_flag(contentArea, LV_OBJ_FLAG_SCROLLABLE);

    CreateTopBar(contentArea, LV_SYMBOL_SETTINGS " System", BackToMenuCb);

    char buf[64];

    settingsManager.getString("device.name", buf, sizeof(buf));
    CreateTextRow(contentArea, "Name", buf, 50, 32);

    settingsManager.getString("ntp.server", buf, sizeof(buf));
    CreateTextRow(contentArea, "NTP", buf, 90, 64);

    settingsManager.getString("ntp.timezone", buf, sizeof(buf));
    CreateTextRow(contentArea, "Timezone", buf, 130, 64);

    snprintf(buf, sizeof(buf), "%ld", settingsManager.getInt("history.rate", 10));
    CreateTextRow(contentArea, "History (s)", buf, 170, 6);

    // Save
    lv_obj_t *saveBtn = CreateButton(contentArea, LV_SYMBOL_OK " Save & Reboot",
                                      lv_palette_main(LV_PALETTE_BLUE), 180, 40, SaveSystemCb);
    lv_obj_align(saveBtn, LV_ALIGN_BOTTOM_RIGHT, -15, -15);
}

// ── Callbacks ────────────────────────────────────────────────

void DisplaySettings::CloseCb(lv_event_t *e)
{
    auto *self = static_cast<DisplaySettings *>(lv_event_get_user_data(e));
    self->Close();
}

void DisplaySettings::BackToMenuCb(lv_event_t *e)
{
    auto *self = static_cast<DisplaySettings *>(lv_event_get_user_data(e));
    self->ShowMenu();
}

void DisplaySettings::WifiBtnCb(lv_event_t *e)
{
    auto *self = static_cast<DisplaySettings *>(lv_event_get_user_data(e));
    self->ShowWifiPage();
}

void DisplaySettings::SensorsBtnCb(lv_event_t *e)
{
    auto *self = static_cast<DisplaySettings *>(lv_event_get_user_data(e));
    self->ShowSensorsPage();
}

void DisplaySettings::SystemBtnCb(lv_event_t *e)
{
    auto *self = static_cast<DisplaySettings *>(lv_event_get_user_data(e));
    self->ShowSystemPage();
}

void DisplaySettings::ScanBtnCb(lv_event_t *e)
{
    auto *self = static_cast<DisplaySettings *>(lv_event_get_user_data(e));
    self->StartWifiScan();
}

void DisplaySettings::WifiNetworkCb(lv_event_t *e)
{
    auto *self = static_cast<DisplaySettings *>(lv_event_get_user_data(e));
    lv_obj_t *btn = lv_event_get_target(e);

    // Get the label child to extract SSID
    lv_obj_t *lbl = lv_obj_get_child(btn, 0);
    if (!lbl)
        return;

    const char *text = lv_label_get_text(lbl);
    // Parse SSID from "lock SSID  (rssi)" format
    // Skip lock symbol if present
    const char *ssidStart = text;
    if (ssidStart[0] == (char)0xEF) // LV_SYMBOL prefix
    {
        // Skip the 3-byte UTF-8 symbol + space
        ssidStart = strchr(text, ' ');
        if (ssidStart)
            ssidStart++;
        else
            ssidStart = text;
    }

    // Find the "  (" delimiter
    const char *end = strstr(ssidStart, "  (");
    if (!end)
        return;

    char ssid[33] = {};
    int len = end - ssidStart;
    if (len > 32)
        len = 32;
    strncpy(ssid, ssidStart, len);

    // Set the SSID in settings
    self->settingsManager.setString("wifi.ssid", ssid);
    ESP_LOGI(TAG, "Selected WiFi: %s", ssid);

    // Refresh the scan list to highlight the new selection
    self->StartWifiScan();
}

void DisplaySettings::SaveWifiCb(lv_event_t *e)
{
    auto *self = static_cast<DisplaySettings *>(lv_event_get_user_data(e));

    // Get password from textarea stored in panel user_data
    lv_obj_t *passTa = static_cast<lv_obj_t *>(lv_obj_get_user_data(self->panel));
    if (passTa)
    {
        const char *pass = lv_textarea_get_text(passTa);
        self->settingsManager.setString("wifi.password", pass);
    }

    self->settingsManager.Save();
    ESP_LOGI(TAG, "WiFi settings saved, rebooting...");
    vTaskDelay(pdMS_TO_TICKS(500));
    esp_restart();
}

void DisplaySettings::ClearSensorsCb(lv_event_t *e)
{
    auto *self = static_cast<DisplaySettings *>(lv_event_get_user_data(e));
    self->sensorManager.ClearAllSlots();
    self->Close();
}

void DisplaySettings::SaveSystemCb(lv_event_t *e)
{
    auto *self = static_cast<DisplaySettings *>(lv_event_get_user_data(e));

    // Walk all textareas in contentArea and save their values
    // TextRows are created with a label + textarea pattern
    // We iterate the contentArea's children to find textareas
    if (!self->contentArea)
        return;

    // Map textarea positions to setting keys per page
    // This is generic: find all textareas and their preceding labels
    uint32_t childCount = lv_obj_get_child_cnt(self->contentArea);
    for (uint32_t i = 0; i < childCount; i++)
    {
        lv_obj_t *child = lv_obj_get_child(self->contentArea, i);
        if (!lv_obj_check_type(child, &lv_textarea_class))
            continue;

        const char *text = lv_textarea_get_text(child);

        // Find the label before this textarea (previous sibling)
        if (i == 0)
            continue;
        lv_obj_t *labelObj = lv_obj_get_child(self->contentArea, i - 1);
        if (!lv_obj_check_type(labelObj, &lv_label_class))
            continue;

        const char *labelText = lv_label_get_text(labelObj);

        // Map label to setting key
        const char *key = nullptr;
        if (strcmp(labelText, "Name") == 0) key = "device.name";
        else if (strcmp(labelText, "NTP") == 0) key = "ntp.server";
        else if (strcmp(labelText, "Timezone") == 0) key = "ntp.timezone";
        else if (strcmp(labelText, "History (s)") == 0) key = "history.rate";
        else if (strcmp(labelText, "Graph Min") == 0) key = "graph.min";
        else if (strcmp(labelText, "Graph Max") == 0) key = "graph.max";
        else if (strcmp(labelText, "Scan (ms)") == 0) key = "sensor.scan";
        else if (strcmp(labelText, "Read (ms)") == 0) key = "sensor.read";

        if (key)
        {
            // Determine if int or string
            const auto *defs = self->settingsManager.GetDefinitions();
            int defCount = self->settingsManager.GetDefinitionCount();
            for (int d = 0; d < defCount; d++)
            {
                if (strcmp(defs[d].key, key) == 0)
                {
                    if (defs[d].type == SettingType::Int)
                        self->settingsManager.setInt(key, atoi(text));
                    else
                        self->settingsManager.setString(key, text);
                    break;
                }
            }
        }
    }

    self->settingsManager.Save();
    ESP_LOGI(TAG, "Settings saved, rebooting...");
    vTaskDelay(pdMS_TO_TICKS(500));
    esp_restart();
}

void DisplaySettings::TextareaFocusCb(lv_event_t *e)
{
    auto *self = static_cast<DisplaySettings *>(lv_event_get_user_data(e));
    lv_obj_t *ta = lv_event_get_target(e);
    self->ShowKeyboard(ta);
}
