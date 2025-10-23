#pragma once
#include <cassert>
#include "ServiceProvider.h"
#include "SettingsManager.h"
#include "DisplayManager.h"
#include "WifiManager.h"
#include "SensorManager.h"
#include "TimeManager.h"
#include "InfluxManager.h"
#include "DataManager.h"
#include "TickContext.h"
#include "WebManager.h"
#include "FatFsDriver.h"
#include "FtpManager.h"
#include "WebUpdateManager.h"

class AppContext : public ServiceProvider
{
    constexpr static const char *TAG = "AppContext";
    constexpr static Milliseconds TICK_WARNING_THRESHOLD = Millis(500);
public:
    AppContext() = default;
    ~AppContext() = default;

    AppContext(const AppContext &) = delete;
    AppContext &operator=(const AppContext &) = delete;

    SensorManager &GetSensorManager() override { return sensorManager; }
    DisplayManager &GetDisplayManager() override { return displayManager; }
    WifiManager &GetWifiManager() override { return wifiManager; }
    // PowerManager&    GetPowerManager() override    { return powerManager; }
    DataManager &GetDataManager() override { return dataManager; }
    InfluxManager &GetInfluxManager() override { return influxManager; }
    TimeManager &GetTimeManager() override { return timeManager; }
    SettingsManager &GetSettingsManager() override { return settingsManager; }
    WebManager &GetWebManager() override { return webManager; }
    FtpManager &GetFtpManager() override { return ftpManager; }
    WebUpdateManager &GetWebUpdateManager() override { return webUpdateManager; }

    void Init()
    {
        fatFsDriver.Init(); 
        GetSettingsManager().Init();
        GetDisplayManager().Init();
        GetWifiManager().Init();
        GetTimeManager().Init();
        GetSensorManager().Init();
        GetInfluxManager().Init();
        GetWebManager().Init();
        GetFtpManager().init();
        GetWebUpdateManager().Init();
        
    }

    void Tick(TickContext& ctx)
    {
        MeasureTick("SettingsManager", [&]() { GetSettingsManager().Tick(ctx); });
        MeasureTick("DisplayManager",  [&]() { GetDisplayManager().Tick(ctx); });
        MeasureTick("WifiManager",     [&]() { GetWifiManager().Tick(ctx); });
        MeasureTick("TimeManager",     [&]() { GetTimeManager().Tick(ctx); });
        MeasureTick("SensorManager",   [&]() { GetSensorManager().Tick(ctx); });
        MeasureTick("InfluxManager",   [&]() { GetInfluxManager().Tick(ctx); });
        MeasureTick("WebManager",      [&]() { GetWebManager().Tick(ctx); });
        MeasureTick("FtpManager",      [&]() { GetFtpManager().Tick(ctx); });
        MeasureTick("WebUpdateManager",[&]() { GetWebUpdateManager().Tick(ctx); });


    }

private:
    FatfsDriver fatFsDriver{"/fat", "fat"};
    SensorManager sensorManager{*this};
    DisplayManager displayManager{*this};
    WifiManager wifiManager{*this};
    // PowerManager    powerManager{*this};
    DataManager dataManager{*this};
    InfluxManager influxManager{*this};
    TimeManager timeManager{*this};
    SettingsManager settingsManager{*this};
    WebManager webManager{*this};
    FtpManager ftpManager{*this};
    WebUpdateManager webUpdateManager{*this};


    template<typename F>
    void MeasureTick(const char* name, F&& func)
    {
        Milliseconds start = NowMs();
        func();
        Milliseconds elapsed = NowMs() - start;

        if (elapsed > TICK_WARNING_THRESHOLD)
        {
            ESP_LOGW("AppContext", "%s Tick took %lld ms", name, elapsed);
        }
    }
};



