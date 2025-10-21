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
    bool IsReady() const { return _initGuard.IsReady(); }
    bool Write(const char *measurement, float value, const DateTime &timestamp, TickType_t timeout);

private:
    SettingsManager &settingsManager;
    InitGuard _initGuard;
    RecursiveMutex _mutex;
    InfluxClient _client;
};
