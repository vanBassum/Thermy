#pragma once
#include "ServiceProvider.h"
#include "CommandManager/CommandManager.h"
#include "DeviceManager/DeviceManager.h"
#include "DisplayManager/DisplayManager.h"
#include "HomeAssistantManager/HomeAssistantManager.h"
#include "ConsoleManager/ConsoleManager.h"
#include "LogManager/LogManager.h"
#include "MqttManager/MqttManager.h"
#include "NetworkManager/NetworkManager.h"
#include "SensorManager/SensorManager.h"
#include "SettingsManager/SettingsManager.h"
#include "MonitorManager/MonitorManager.h"
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
    DeviceManager& getDeviceManager() override { return m_deviceManager; }
    DisplayManager& getDisplayManager() override { return m_displayManager; }
    HomeAssistantManager& getHomeAssistantManager() override { return m_homeAssistantManager; }
    ConsoleManager& getConsoleManager() override { return m_consoleManager; }
    LogManager& getLogManager() override { return m_logManager; }
    MqttManager& getMqttManager() override { return m_mqttManager; }
    NetworkManager& getNetworkManager() override { return m_networkManager; }
    SensorManager& getSensorManager() override { return m_sensorManager; }
    SettingsManager& getSettingsManager() override { return m_settingsManager; }
    MonitorManager& getMonitorManager() override { return m_monitorManager; }
    TimeManager& getTimeManager() override { return m_timeManager; }
    UpdateManager& getUpdateManager() override { return m_updateManager; }
    WebServerManager& getWebServerManager() override { return m_webServerManager; }

private:
    ConsoleManager m_consoleManager{*this};
    LogManager m_logManager{*this};
    SettingsManager m_settingsManager{*this};
    NetworkManager m_networkManager{*this};
    SensorManager m_sensorManager{*this};
    TimeManager m_timeManager{*this};
    MonitorManager m_monitorManager{*this};
    DisplayManager m_displayManager{*this};
    CommandManager m_commandManager{*this};
    MqttManager m_mqttManager{*this};
    DeviceManager m_deviceManager{*this};
    HomeAssistantManager m_homeAssistantManager{*this};
    UpdateManager m_updateManager{*this};
    WebServerManager m_webServerManager{*this};
};
