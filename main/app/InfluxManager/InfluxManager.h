#pragma once
#include "ServiceProvider.h"
#include "InitGuard.h"
#include "RecursiveMutex.h"
#include "InfluxClient.h"
#include "DateTime.h"
#include "esp_log.h"
#include "SettingsManager.h"

class InfluxManager
{
    inline static constexpr const char *TAG = "InfluxManager";

public:
    explicit InfluxManager(ServiceProvider &ctx);

    void Init();
    bool Write(const char *measurement, float value, const DateTime &timestamp, TickType_t timeout);
    InfluxSession BeginWrite(const char *measurement, const DateTime &timestamp, TickType_t timeout)
    {
        REQUIRE_READY(_initGuard);
        LOCK(_mutex);
        return _client.Measurement(measurement, timestamp, timeout);
    }

private:
    SettingsManager &settingsManager;
    InitGuard _initGuard;
    RecursiveMutex _mutex;
    InfluxClient _client;

    char influxBaseUrl[128];
    char influxApiKey[128];
    char influxOrganisation[64];
    char influxBucket[64]; 
};
