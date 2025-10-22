#include "InfluxManager.h"
#include "core_utils.h"

InfluxManager::InfluxManager(ServiceProvider &services)
    : settingsManager(services.GetSettingsManager())
    , dataManager(services.GetDataManager())
    , wifiManager(services.GetWifiManager())
    , timeManager(services.GetTimeManager())
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

void InfluxManager::Tick(TickContext &ctx)
{
    REQUIRE_READY(_initGuard);
    LOCK(_mutex);

    if (!ctx.HasElapsed(_lastWriteTime, INFLUX_WRITE_INTERVAL))
        return;

    if (!wifiManager.IsConnected()) {
        ctx.PreventSleep(); // Prevent sleep for wifi to reconnect
        return;
    }

    if (!timeManager.IsTimeValid()){
        ctx.PreventSleep(); // Prevent sleep for ntp to sync
        return;
    }

    ESP_LOGI(TAG, "Writing data to InfluxDB...");


    
    _lastWriteTime = ctx.TimeSinceBoot();
}


