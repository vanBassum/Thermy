#pragma once
#include "AppProvider.h"
#include "InitState.h"
#include "TypedSettings.h"
#include "rtos.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "lvgl.h"

#include "HomePage.h"
#include "SettingsMenuPage.h"
#include "WifiPage.h"
#include "SensorPage.h"
#include "GraphPage.h"
#include "SystemPage.h"

// ──────────────────────────────────────────────────────────────
// The local touch UI: LVGL on the board's panel, a stack of pages, and the
// "new probe found — where is it?" popup.
//
// The panel itself belongs to the board (BoardContext::GetDisplay); this manager
// only drives LVGL on top of it. That is why it initialises after the board layer.
// ──────────────────────────────────────────────────────────────
class DisplayManager
{
    inline static constexpr const char *TAG = "DisplayManager";
    static constexpr int LCD_HRES = 480;
    static constexpr int LCD_VRES = 320;
    static constexpr TickType_t POPUP_TIMEOUT = pdMS_TO_TICKS(30000);

public:
    explicit DisplayManager(AppProvider &app);

    DisplayManager(const DisplayManager &) = delete;
    DisplayManager &operator=(const DisplayManager &) = delete;
    DisplayManager(DisplayManager &&) = delete;
    DisplayManager &operator=(DisplayManager &&) = delete;

    void Init();

private:
    AppProvider &app_;
    SensorManager &sensorManager;
    InitState initState;
    Task task;
    esp_timer_handle_t lvglTickTimer = nullptr;

    // Pages
    HomePage homePage;
    SettingsMenuPage settingsMenuPage;
    WifiPage wifiPage;
    SensorPage sensorPage;
    GraphPage graphPage;
    SystemPage systemPage;
    DisplayPage *activePage = nullptr;

    // Sensor assignment popup
    lv_obj_t *assignPopup = nullptr;
    uint64_t popupSensorAddress = 0;
    TickType_t popupShownAt = 0;

    void Work();
    static void LvglTickCb(void *arg);
    void NavigateTo(const char *page);

    // Popup
    void ShowAssignPopup(uint64_t address);
    void CloseAssignPopup();
    void OnSlotSelected(int slot);
    void AutoAssignToFirstEmpty();
    static void PopupEventCb(lv_event_t *e);

    // ── Settings (registered with SettingsManager in Init) ──
    // The on-device chart's Y range. Owned here because the chart is this
    // manager's, and read by HomePage/GraphPage through the registry.
    inline static Int32Setting graphMin_{ "graph.min", "Graph Y Min (°C)", 0 };
    inline static Int32Setting graphMax_{ "graph.max", "Graph Y Max (°C)", 100 };
};
