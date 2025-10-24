#pragma once

// Forward declarations
class SensorManager;
class DisplayManager;
class WifiManager;
// class PowerManager;
class DataManager;
class InfluxManager;
class TimeManager;
class SettingsManager;
class WebManager;
class FtpManager;
class WebUpdateManager;
class SimpleStats;

class ServiceProvider
{
public:
    virtual SensorManager &GetSensorManager() = 0;
    virtual DisplayManager &GetDisplayManager() = 0;
    virtual WifiManager &GetWifiManager() = 0;
    // virtual PowerManager&    GetPowerManager() = 0;
    virtual DataManager &GetDataManager() = 0;
    virtual InfluxManager &GetInfluxManager() = 0;
    virtual TimeManager &GetTimeManager() = 0;
    virtual SettingsManager &GetSettingsManager() = 0;
    virtual WebManager &GetWebManager() = 0;
    virtual FtpManager &GetFtpManager() = 0;
    virtual WebUpdateManager &GetWebUpdateManager() = 0;



    virtual SimpleStats* GetStats() = 0;
    virtual int GetStatsCount() = 0;
    
};
