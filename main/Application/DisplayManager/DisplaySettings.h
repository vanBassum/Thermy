#pragma once
#include "lvgl.h"

class SettingsManager;
class SensorManager;
class NetworkManager;

class DisplaySettings
{
    inline static constexpr const char *TAG = "DisplaySettings";
    static constexpr int LCD_HRES = 480;
    static constexpr int LCD_VRES = 320;

public:
    DisplaySettings(SettingsManager &settings, SensorManager &sensors, NetworkManager &network);

    void Show(lv_obj_t *parent);
    void Close();
    bool IsOpen() const { return panel != nullptr; }

private:
    SettingsManager &settingsManager;
    SensorManager &sensorManager;
    NetworkManager &networkManager;

    lv_obj_t *panel = nullptr;
    lv_obj_t *contentArea = nullptr;
    lv_obj_t *keyboard = nullptr;

    // Reusable UI helpers
    lv_obj_t *CreatePanel(lv_obj_t *parent);
    lv_obj_t *CreateTopBar(lv_obj_t *parent, const char *title, lv_event_cb_t backCb);
    lv_obj_t *CreateButton(lv_obj_t *parent, const char *text, lv_color_t color,
                           lv_coord_t w, lv_coord_t h, lv_event_cb_t cb);
    lv_obj_t *CreateTextRow(lv_obj_t *parent, const char *label, const char *value,
                            lv_coord_t y, int maxLen, bool password = false);
    void ShowKeyboard(lv_obj_t *textarea);
    void HideKeyboard();

    // Pages
    void ShowMenu();
    void ShowWifiPage();
    void ShowSensorsPage();
    void ShowSystemPage();

    void ClearContent();

    // WiFi scan
    void StartWifiScan();

    // Callbacks
    static void BackToMenuCb(lv_event_t *e);
    static void CloseCb(lv_event_t *e);
    static void WifiBtnCb(lv_event_t *e);
    static void SensorsBtnCb(lv_event_t *e);
    static void SystemBtnCb(lv_event_t *e);
    static void WifiNetworkCb(lv_event_t *e);
    static void SaveWifiCb(lv_event_t *e);
    static void ClearSensorsCb(lv_event_t *e);
    static void SaveSystemCb(lv_event_t *e);
    static void TextareaFocusCb(lv_event_t *e);
    static void ScanBtnCb(lv_event_t *e);
};
