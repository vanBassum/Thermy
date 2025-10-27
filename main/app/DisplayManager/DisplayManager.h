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
#include "InfluxManager.h"
#include "HardwareManager.h"


class DisplayManager
{
    inline static constexpr const char *TAG = "DisplayManager";
    inline static constexpr Milliseconds DISPLAY_UPDATE_INTERVAL = Millis(500);

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
    DisplayDriver driver;
    Milliseconds lastDisplayUpdate;
    int rotator = 0;

    void DrawIcons(SSD1306 &display);
    void DrawSensorTemperatures(SSD1306 &display);
};

