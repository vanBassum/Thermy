#include "InfluxManager.h"
#include "core_utils.h"

InfluxManager::InfluxManager(ServiceProvider &services)
    : settingsManager(services.GetSettingsManager())
{
}

void InfluxManager::Init()
{
    if (_initGuard.IsReady())
        return;

    LOCK(_mutex);

    settingsManager.Access([this](const RootSettings &settings)
    {
        memcpy(influxBaseUrl, settings.system.influxBaseUrl, MIN(sizeof(influxBaseUrl), sizeof(settings.system.influxBaseUrl)));
        memcpy(influxApiKey, settings.system.influxApiKey, MIN(sizeof(influxApiKey), sizeof(settings.system.influxApiKey)));
        memcpy(influxOrganisation, settings.system.influxOrganisation, MIN(sizeof(influxOrganisation), sizeof(settings.system.influxOrganisation)));
        memcpy(influxBucket, settings.system.influxBucket, MIN(sizeof(influxBucket), sizeof(settings.system.influxBucket)));
    });

    _client.Init(influxBaseUrl, influxApiKey, influxOrganisation, influxBucket);
    _initGuard.SetReady();
}

bool InfluxManager::Write(const char *measurement, float value, const DateTime &timestamp, TickType_t timeout)
{
    REQUIRE_READY(_initGuard);
    LOCK(_mutex);

    bool success = _client.Measurement(measurement, timestamp, timeout)
        .withField("value", value)
        .Finish();
    return success;
}
