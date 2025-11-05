#pragma once
#include <cassert>
#include "DisplayManager.h"
#include "FtpManager.h"
#include "SensorManager.h"
#include "ServiceProvider.h"
#include "SettingsManager.h"
#include "SimpleStats.h"
#include "TimeManager.h"
#include "WebManager.h"

class AppContext : public ServiceProvider
{
    constexpr static const char *TAG = "AppContext";

public:
    AppContext() = default;
    ~AppContext() = default;

    AppContext(const AppContext &) = delete;
    AppContext &operator=(const AppContext &) = delete;

    virtual DisplayManager &GetDisplayManager() override { return displayManager; }
    virtual FtpManager &GetFtpManager() override { return ftpManager; }
    virtual SensorManager &GetSensorManager() override { return sensorManager; }
    virtual SettingsManager &GetSettingsManager() override { return settingsManager; }
    virtual TimeManager &GetTimeManager() override { return timeManager; }
    virtual WebManager &GetWebManager() override { return webManager; }
    virtual WifiManager &GetWifiManager() override { return wifiManager; }

    void Init()
    {
        GetSettingsManager().Init();
        GetDisplayManager().Init();
        GetWifiManager().Init();
        GetSensorManager().Init();
        GetTimeManager().Init();
        GetWebManager().Init();
        GetFtpManager().Init();
    }

private:
    DisplayManager displayManager{*this};
    FtpManager ftpManager{*this};
    SensorManager sensorManager{*this};
    SettingsManager settingsManager{*this};
    TimeManager timeManager{*this};
    WebManager webManager{*this};
    WifiManager wifiManager{*this};
};
