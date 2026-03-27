#include "MonitorManager.h"
#include "LogManager/LogManager.h"
#include "SensorManager/SensorManager.h"
#include "SettingsManager/SettingsManager.h"
#include "esp_log.h"

MonitorManager::MonitorManager(ServiceProvider& serviceProvider)
    : serviceProvider_(serviceProvider)
    , logManager_(serviceProvider.getLogManager())
    , sensorManager_(serviceProvider.getSensorManager())
    , settingsManager_(serviceProvider.getSettingsManager())
{
}

void MonitorManager::Init()
{
    auto initAttempt = initState_.TryBeginInit();
    if (!initAttempt)
    {
        return;
    }

    int32_t rateSec = settingsManager_.getInt("monitor.rate", DEFAULT_RATE_SECONDS);
    if (rateSec < 1) rateSec = 1;

    sampleTimer_.Init("Monitor", pdMS_TO_TICKS(rateSec * 1000), true);
    sampleTimer_.SetHandler([this]() { TakeSample(); });
    sampleTimer_.Start();

    initAttempt.SetReady();
    ESP_LOGI(TAG, "Initialized (rate: %lds)", rateSec);
}

void MonitorManager::TakeSample()
{
    logManager_.Append(
        LogKeys::TimeStamp, DateTime::Now(),
        LogKeys::LogCode, static_cast<uint32_t>(LogCode::TemperatureReading),
        LogKeys::Temperature_1, sensorManager_.GetTemperature(0),
        LogKeys::Temperature_2, sensorManager_.GetTemperature(1),
        LogKeys::Temperature_3, sensorManager_.GetTemperature(2),
        LogKeys::Temperature_4, sensorManager_.GetTemperature(3)
    );

}

