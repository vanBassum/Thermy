#pragma once

class CommandManager;
class DisplayManager;
class LogManager;
class NetworkManager;
class SensorManager;
class SettingsManager;
class TimeManager;
class UpdateManager;
class WebServerManager;

class ServiceProvider
{
public:
    virtual CommandManager& getCommandManager() = 0;
    virtual DisplayManager& getDisplayManager() = 0;
    virtual LogManager& getLogManager() = 0;
    virtual NetworkManager& getNetworkManager() = 0;
    virtual SensorManager& getSensorManager() = 0;
    virtual SettingsManager& getSettingsManager() = 0;
    virtual TimeManager& getTimeManager() = 0;
    virtual UpdateManager& getUpdateManager() = 0;
    virtual WebServerManager& getWebServerManager() = 0;
};
