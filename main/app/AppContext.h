#pragma once
#include <cassert>
#include "ServiceProvider.h"
#include "SettingsManager.h"
#include "DisplayManager.h"
#include "WifiManager.h"    
#include "SensorManager.h"


class AppContext : public ServiceProvider
{
public:
    AppContext() = default;
    ~AppContext() = default;

    AppContext(const AppContext&) = delete;
    AppContext& operator=(const AppContext&) = delete;

    SensorManager&   GetSensorManager() override   { return sensorManager; }
    DisplayManager&  GetDisplayManager() override  { return displayManager; }
    WifiManager&     GetWifiManager() override     { return wifiManager; }
    //PowerManager&    GetPowerManager() override    { return powerManager; }
    //DataLogger&      GetDataLogger() override      { return dataLogger; }
    //InfluxClient&    GetInfluxClient() override    { return influxClient; }
    //TimeManager&     GetTimeManager() override     { return timeManager; }
    SettingsManager& GetSettingsManager() override { return settingsManager; }

private:
    SensorManager   sensorManager{*this};
    DisplayManager  displayManager{*this};
    WifiManager     wifiManager{*this};
    //PowerManager    powerManager{*this};
    //DataLogger      dataLogger{*this};
    //InfluxClient    influxClient{*this};
    //TimeManager     timeManager{*this};
    SettingsManager settingsManager{*this};


};
