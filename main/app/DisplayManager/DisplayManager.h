#pragma once
#include "ServiceProvider.h"
#include "rtos.h"
#include "esp_log.h"
#include "Display_WT32SC01.h"
#include "lvgl.h"
#include "WifiManager.h"

class DisplayManager
{
    inline static constexpr const char *TAG = "DisplayManager";

    static constexpr int LCD_HRES = Display_WT32SC01::LCD_HRES;
    static constexpr int LCD_VRES = Display_WT32SC01::LCD_VRES;

public:
    explicit DisplayManager(ServiceProvider &ctx);
    void Init();

private:
    WifiManager &wifiManager;
    SensorManager &sensorManager;

    InitGuard initGuard;
    Mutex mutex;
    Display_WT32SC01 display;
    Task task;
    Timer timer;
    int chartCounter = 0;

    lv_obj_t *labelTime = nullptr;
    lv_obj_t *labelIP = nullptr;
    lv_obj_t *tempBoxes[4] = {nullptr};
    lv_obj_t *tempLabels[4] = {nullptr};
    lv_obj_t *chart = nullptr;
    lv_chart_series_t *chartSeries[4] = {nullptr};

    void Work();
    void LvglTickCb(void *arg);

    void UiSetup();
    void UiUpdate();

};
