#include "InfluxManager.h"
#include "secrets.h"

InfluxManager::InfluxManager(ServiceProvider &services)
    : settingsManager(services.GetSettingsManager())
{
}

void InfluxManager::Init()
{
    if (_initGuard.IsReady())
        return;

    LOCK(_mutex);

    const char *baseUrl = INFLUX_BASE_URL;
    const char *apiKey = INFLUX_API_KEY;
    const char *organization = INFLUX_ORGANISATION;
    const char *bucket = INFLUX_BUCKET;
    
    if (!baseUrl || !apiKey || !organization || !bucket)
    {
        ESP_LOGW(TAG, "Influx settings incomplete, skipping init");
        return;
    }

    _client.Init(baseUrl, apiKey, organization, bucket);
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
