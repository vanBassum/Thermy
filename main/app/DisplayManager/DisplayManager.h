#pragma once
#include "ServiceProvider.h"
#include "rtos.h"
#include "esp_log.h"
#include "TickContext.h"
#include "Display_WT32SC01.h"
#include "lvgl.h"

class DisplayManager
{
    inline static constexpr const char *TAG = "DisplayManager";

public:
    explicit DisplayManager(ServiceProvider &ctx);
    void Init();

private:
    InitGuard initGuard;
    Mutex mutex;
    Display_WT32SC01 display;
    Task task;
    Timer timer;

    void Work();
    void LvglTickCb(void *arg);
};
