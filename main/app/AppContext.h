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


class AppContext : public ServiceProvider
{
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

    void Init()
    {
        GetSettingsManager().Init();
        GetDisplayManager().Init();
        GetWifiManager().Init();
        GetTimeManager().Init();
        GetSensorManager().Init();
        GetInfluxManager().Init();
    }

    void Tick(TickContext& ctx) {
        GetSettingsManager().Tick(ctx);
        GetDisplayManager() .Tick(ctx);
        GetWifiManager()    .Tick(ctx);
        GetTimeManager()    .Tick(ctx);
        GetSensorManager()  .Tick(ctx);
        GetInfluxManager()  .Tick(ctx);
    }

private:
    SensorManager sensorManager{*this};
    DisplayManager displayManager{*this};
    WifiManager wifiManager{*this};
    // PowerManager    powerManager{*this};
    DataManager dataManager{*this};
    InfluxManager influxManager{*this};
    TimeManager timeManager{*this};
    SettingsManager settingsManager{*this};
};



