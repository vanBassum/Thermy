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
public:
    AppContext() = default;
    ~AppContext() = default;

    AppContext(const AppContext &) = delete;
    AppContext &operator=(const AppContext &) = delete;

    SensorManager &GetSensorManager() override { return sensorManager; }
    DisplayManager &GetDisplayManager() override { return displayManager; }
    WifiManager &GetWifiManager() override { return wifiManager; }
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
private:
    FatfsDriver fatFsDriver{"/fat", "fat"};
    SensorManager sensorManager{*this};
    DisplayManager displayManager{*this};
    WifiManager wifiManager{*this};
    DataManager dataManager{*this};
    InfluxManager influxManager{*this};
    TimeManager timeManager{*this};
    SettingsManager settingsManager{*this};
    WebManager webManager{*this};
    FtpManager ftpManager{*this};
    WebUpdateManager webUpdateManager{*this};
};



