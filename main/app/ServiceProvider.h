#pragma once

// Forward declarations
class DisplayManager;
class FtpManager;
class SensorManager;
class SettingsManager;
class SimpleStats;
class TimeManager;
class WebManager;
class WifiManager;

class ServiceProvider
{
public:
    virtual DisplayManager &GetDisplayManager() = 0;
    virtual FtpManager &GetFtpManager() = 0;
    virtual SensorManager &GetSensorManager() = 0;
    virtual SettingsManager &GetSettingsManager() = 0;
    virtual TimeManager &GetTimeManager() = 0;
    virtual WebManager &GetWebManager() = 0;
    virtual WifiManager &GetWifiManager() = 0;
};
