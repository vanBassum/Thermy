#pragma once
#include "ServiceProvider.h"
#include "InitGuard.h"
#include "RecursiveMutex.h"
#include "InfluxClient.h"
#include "DateTime.h"
#include "esp_log.h"
#include "SettingsManager.h"
#include "TickContext.h"
#include "DataManager.h"
#include "WifiManager.h"
#include "TimeManager.h"

class InfluxManager
{
    inline static constexpr const char *TAG = "InfluxManager";
    inline static constexpr Milliseconds INFLUX_WRITE_INTERVAL = Millis(30000);

public:
    explicit InfluxManager(ServiceProvider &ctx);

    void Init();
    void Tick(TickContext& ctx);

private:
    SettingsManager &settingsManager;
    DataManager &dataManager;
    WifiManager &wifiManager;
    TimeManager &timeManager;

    InitGuard _initGuard;
    RecursiveMutex _mutex;
    InfluxClient _client;
    Milliseconds _lastWriteTime = 0;

    char influxBaseUrl[128];
    char influxApiKey[128];
    char influxOrganisation[64];
    char influxBucket[64]; 
};
