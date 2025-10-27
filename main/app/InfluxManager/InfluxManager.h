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
#include "rtos.h"

class InfluxManager
{
    inline static constexpr const char *TAG = "InfluxManager";
    inline static constexpr Milliseconds INFLUX_WRITE_INTERVAL = Millis(10000);

    enum class State
    {
        Idle,
        Working
    };

public:
    explicit InfluxManager(ServiceProvider &ctx);

    void Init();
    void Tick(TickContext &ctx);
    bool IsWorking() const { return _state.Get() == State::Working; }

private:
    SettingsManager &settingsManager;
    DataManager &dataManager;
    WifiManager &wifiManager;
    TimeManager &timeManager;

    Task task;
    InitGuard _initGuard;
    RecursiveMutex _mutex;
    Milliseconds _lastWriteTime = 0;
    Synchronized<State> _state{State::Idle};
    
    char influxBaseUrl[128];
    char influxApiKey[128];
    char influxOrganisation[64];
    char influxBucket[64];



    void Work();    

    void EnsureValidTimestamp(DataEntry &entry);
    void SendLogCode_Temperature(InfluxSession &session, DataEntry &entry);
};
