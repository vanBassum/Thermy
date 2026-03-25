#pragma once
#include "ServiceProvider.h"
#include "rtos.h"
#include "esp_log.h"
#include "Display_WT32SC01.h"
#include "lvgl.h"
#include "NetworkManager/NetworkManager.h"
#include "SensorManager/SensorManager.h"

class DisplayManager
{
    inline static constexpr const char *TAG = "DisplayManager";

    static constexpr int LCD_HRES = Display_WT32SC01::LCD_HRES;
    static constexpr int LCD_VRES = Display_WT32SC01::LCD_VRES;
    static constexpr TickType_t POPUP_TIMEOUT = pdMS_TO_TICKS(30000);

public:
    explicit DisplayManager(ServiceProvider &ctx);
    void Init();

private:
    NetworkManager &networkManager;
    SensorManager &sensorManager;

    InitState initState;
    Display_WT32SC01 display;
    Task task;
    Timer timer;
    int chartCounter = 0;

    // Main UI
    lv_obj_t *labelTime = nullptr;
    lv_obj_t *labelIP = nullptr;
    lv_obj_t *tempBoxes[4] = {nullptr};
    lv_obj_t *tempLabels[4] = {nullptr};
    lv_obj_t *chart = nullptr;
    lv_chart_series_t *chartSeries[4] = {nullptr};

    // Assignment popup
    lv_obj_t *assignPopup = nullptr;
    uint64_t popupSensorAddress = 0;
    TickType_t popupShownAt = 0;

    void Work();
    void LvglTickCb(void *arg);

    void UiSetup();
    void UiUpdate();

    void ShowAssignPopup(uint64_t address);
    void CloseAssignPopup();
    void OnSlotSelected(int slot);
    void AutoAssignToFirstEmpty();

    static void PopupEventCb(lv_event_t *e);
};
