#pragma once

// Forward declarations
//class SensorManager;
class DisplayManager;
class WifiManager;
//class PowerManager;
//class DataLogger;
//class InfluxClient;
//class TimeManager;
class SettingsManager;

class ServiceProvider
{
public:
    //virtual SensorManager&   GetSensorManager() = 0;
    virtual DisplayManager&  GetDisplayManager() = 0;
    virtual WifiManager&     GetWifiManager() = 0;
    //virtual PowerManager&    GetPowerManager() = 0;
    //virtual DataLogger&      GetDataLogger() = 0;
    //virtual InfluxClient&    GetInfluxClient() = 0;
    //virtual TimeManager&     GetTimeManager() = 0;
    virtual SettingsManager& GetSettingsManager() = 0;
};
