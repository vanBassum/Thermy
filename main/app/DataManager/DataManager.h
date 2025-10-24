#pragma once
#include "ServiceProvider.h"
#include "esp_log.h"
#include "DataEntry.h"
#include "TickContext.h"
#include "SensorManager.h"
#include "RingBuffer.h"

class DataManager
{
    inline static constexpr const char *TAG = "DataManager";
    constexpr static const size_t MAX_ENTRIES = 1000;
    constexpr static const Milliseconds TEMPERATURE_LOG_INTERVAL = Millis(2500);

public:
    using Buffer = RingBuffer<DataEntry, MAX_ENTRIES>;
    using Iterator = typename Buffer::Iterator;

    explicit DataManager(ServiceProvider &ctx)
        : sensorManager(ctx.GetSensorManager())
    {
    }

    void Init() {}

    void Tick(TickContext &ctx)
    {
        if (ctx.HasElapsed(lastTemperatureLog, TEMPERATURE_LOG_INTERVAL))
        {
            LogTemperatures();
            ctx.MarkExecuted(lastTemperatureLog, TEMPERATURE_LOG_INTERVAL);
        }
    }

    // ---- Iterator access ----
    Iterator GetIterator()
    {
        
        return ringBuffer.GetIterator();
    }

    // ---- Append new entry ----
    void Append(const DataEntry &entry)
    {
        ringBuffer.Append(entry);
    }

private:
    SensorManager &sensorManager;
    Buffer ringBuffer;
    Milliseconds lastTemperatureLog = 0;

    void LogTemperatures()
    {
        const int cnt = sensorManager.GetSensorCount();
        for (int i = 0; i < cnt; ++i)
        {
            DataEntry entry;
            entry.timestamp = DateTime::Now();
            entry.pairs[0] = { DataKey::LogCode, LogCode::TemperatureRead };
            entry.pairs[1] = { DataKey::Address,  sensorManager.GetAddress(i) };
            entry.pairs[2] = { DataKey::Value,    sensorManager.GetTemperature(i) };
            Append(entry);
        }
    }
};
