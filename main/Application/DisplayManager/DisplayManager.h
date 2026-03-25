#pragma once
#include "ServiceProvider.h"
#include "rtos.h"
#include "esp_log.h"
#include "Display_WT32SC01.h"
#include "lvgl.h"

#include "HomePage.h"
#include "SettingsMenuPage.h"
#include "WifiPage.h"
#include "SensorPage.h"
#include "GraphPage.h"
#include "SystemPage.h"

class DisplayManager
{
    inline static constexpr const char *TAG = "DisplayManager";
    static constexpr int LCD_HRES = 480;
    static constexpr int LCD_VRES = 320;
    static constexpr TickType_t POPUP_TIMEOUT = pdMS_TO_TICKS(30000);

public:
    explicit DisplayManager(ServiceProvider &ctx);
    void Init();

private:
    SensorManager &sensorManager;
    InitState initState;
    Display_WT32SC01 display;
    Task task;
    Timer timer;

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
    void LvglTickCb(void *arg);
    void NavigateTo(const char *page);

    // Popup
    void ShowAssignPopup(uint64_t address);
    void CloseAssignPopup();
    void OnSlotSelected(int slot);
    void AutoAssignToFirstEmpty();
    static void PopupEventCb(lv_event_t *e);
};
