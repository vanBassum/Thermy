#pragma once
#include "ServiceProvider.h"
#include "InitGuard.h"
#include "Mutex.h"
#include "esp_log.h"
#include "TickContext.h"
#include "SensorManager.h"
#include "Display.h"
#include "WifiManager.h"
#include "TimeManager.h"
#include "InfluxManager.h"
#include "HardwareManager.h"


class DisplayManager
{
    inline static constexpr const char *TAG = "DisplayManager";
    inline static constexpr Milliseconds DISPLAY_UPDATE_INTERVAL = Millis(10000);

public:
    explicit DisplayManager(ServiceProvider &ctx);

    void Init();
    void Tick(TickContext& ctx);

private:
    SensorManager& sensorManager;
    WifiManager& wifiManager;
    TimeManager& timeManager;
    InfluxManager& influxManager;
    HardwareManager& hardwareManager;

    InitGuard initGuard;
    Mutex mutex;
    Milliseconds lastDisplayUpdate;
    int rotator = 0;

    Display_SSD1680 display;

    void DrawIcons(Display &display);
    void DrawSensorTemperatures(Display &display);
};

