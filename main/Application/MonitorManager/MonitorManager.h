#pragma once

#include "ServiceProvider.h"
#include "InitState.h"
#include "Timer.h"

class LogManager;
class SensorManager;
class SettingsManager;

class MonitorManager
{
    static constexpr const char* TAG = "MonitorManager";

public:
    static constexpr int32_t DEFAULT_RATE_SECONDS = 10;

    explicit MonitorManager(ServiceProvider& serviceProvider);

    MonitorManager(const MonitorManager&) = delete;
    MonitorManager& operator=(const MonitorManager&) = delete;

    void Init();

private:
    ServiceProvider& serviceProvider_;
    LogManager& logManager_;
    SensorManager& sensorManager_;
    SettingsManager& settingsManager_;
    InitState initState_;
    Timer sampleTimer_;

    void TakeSample();
};
