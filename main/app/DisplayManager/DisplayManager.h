#pragma once
#include "ServiceProvider.h"
#include "InitGuard.h"
#include "Mutex.h"
#include "esp_log.h"
#include "TickContext.h"
#include "SensorManager.h"
#include "DisplayDriver.h"
#include "WifiManager.h"
#include "TimeManager.h"


class DisplayManager
{
    inline static constexpr const char *TAG = "DisplayManager";
    inline static constexpr Milliseconds DISPLAY_UPDATE_INTERVAL = Millis(1000);

public:
    explicit DisplayManager(ServiceProvider &ctx);

    void Init();
    void Tick(TickContext& ctx);

private:
    SensorManager& sensorManager;
    WifiManager& wifiManager;
    TimeManager& timeManager;
    InitGuard initGuard;
    Mutex mutex;
    DisplayDriver driver;
    Milliseconds lastDisplayUpdate;

    void DrawIcons(SSD1306 &display);
    void DrawSensorTemperatures(SSD1306 &display);
};

