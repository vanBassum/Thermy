#pragma once
#include "ServiceProvider.h"
#include "CommandManager/CommandManager.h"
#include "DisplayManager/DisplayManager.h"
#include "LogManager/LogManager.h"
#include "NetworkManager/NetworkManager.h"
#include "SensorManager/SensorManager.h"
#include "SettingsManager/SettingsManager.h"
#include "TimeManager/TimeManager.h"
#include "UpdateManager/UpdateManager.h"
#include "WebServerManager/WebServerManager.h"

class ApplicationContext : public ServiceProvider
{
public:
    ApplicationContext() = default;
    ~ApplicationContext() = default;
    ApplicationContext(const ApplicationContext&) = delete;
    ApplicationContext& operator=(const ApplicationContext&) = delete;

    CommandManager& getCommandManager() override { return m_commandManager; }
    DisplayManager& getDisplayManager() override { return m_displayManager; }
    LogManager& getLogManager() override { return m_logManager; }
    NetworkManager& getNetworkManager() override { return m_networkManager; }
    SensorManager& getSensorManager() override { return m_sensorManager; }
    SettingsManager& getSettingsManager() override { return m_settingsManager; }
    TimeManager& getTimeManager() override { return m_timeManager; }
    UpdateManager& getUpdateManager() override { return m_updateManager; }
    WebServerManager& getWebServerManager() override { return m_webServerManager; }

private:
    LogManager m_logManager{*this};
    SettingsManager m_settingsManager{*this};
    NetworkManager m_networkManager{*this};
    SensorManager m_sensorManager{*this};
    TimeManager m_timeManager{*this};
    DisplayManager m_displayManager{*this};
    CommandManager m_commandManager{*this};
    UpdateManager m_updateManager{*this};
    WebServerManager m_webServerManager{*this};
};
