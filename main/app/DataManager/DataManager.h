#pragma once
#include "ServiceProvider.h"
#include "rtos.h"
#include "esp_log.h"
#include "DataEntry.h"
#include "TickContext.h"
#include "SensorManager.h"


class DataManager
{
    constexpr static const size_t MAX_ENTRIES = 200;
    inline static constexpr const char *TAG = "DataManager";
    constexpr static const Milliseconds TEMPERATURE_LOG_INTERVAL = Millis(2500);

public:
    explicit DataManager(ServiceProvider &ctx)  
        : sensorManager(ctx.GetSensorManager())
    {

    }

    void Init() {}

    void Tick(TickContext& ctx) {

        if(ctx.HasElapsed(lastTemperatureLog, TEMPERATURE_LOG_INTERVAL))
        {
            LogTemperatures();
            ctx.MarkExecuted(lastTemperatureLog, TEMPERATURE_LOG_INTERVAL);
        }
    }

    template <typename CallbackFunc>
    void ForEach(CallbackFunc callback)
    {
        LOCK(mutex);

        for (size_t i = 0; i < MAX_ENTRIES; ++i)
        {
            size_t index = (writePtr + i) % MAX_ENTRIES;
            DataEntry &entry = entries[index];
            if (entry.timestamp != DateTime::MinValue())
                callback(entry);
        }
    }

private:
    SensorManager &sensorManager;

    Mutex mutex;
    DataEntry entries[MAX_ENTRIES] = {};
    size_t writePtr = 0; // Always points to empty
    Milliseconds lastTemperatureLog = 0;


    void LogTemperatures()
    {
        int cnt = sensorManager.GetSensorCount();
        for(int i =0; i<cnt; ++i)
        {
            uint64_t address = sensorManager.GetAddress(i);
            float temperature = sensorManager.GetTemperature(i);
            DataEntry entry;
            entry.timestamp = DateTime::Now();
            entry.pairs[0] = { DataKey::LogCode, LogCode::TemperatureRead };
            entry.pairs[1] = { DataKey::Address, address};
            entry.pairs[2] = { DataKey::Value, temperature};
            Append(entry);
        }
    }


    // FIFO append
    void Append(const DataEntry &entry)
    {
        entries[writePtr] = entry;
        writePtr = (writePtr + 1) % MAX_ENTRIES;
    }
};
