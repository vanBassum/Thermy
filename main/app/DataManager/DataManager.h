#pragma once
#include "ServiceProvider.h"
#include "rtos.h"
#include "esp_log.h"
#include "DataEntry.h"
#include "TickContext.h"


class DataManager
{
    constexpr static const size_t MAX_ENTRIES = 100;
    inline static constexpr const char *TAG = "DataManager";

public:
    explicit DataManager(ServiceProvider &ctx){}
    void Init() {}
    void Tick(TickContext& ctx) {}

    // FIFO append
    void Append(const DataEntry &entry)
    {
        LOCK(mutex);

        entries[writePtr] = entry;
        writePtr = (writePtr + 1) % MAX_ENTRIES;
    }

    template <typename FinderFunc, typename CallbackFunc>
    bool FindEntry(FinderFunc finder, CallbackFunc callback)
    {
        LOCK(mutex);

        for (size_t i = 0; i < MAX_ENTRIES; ++i)
        {
            size_t index = (writePtr + i) % MAX_ENTRIES;
            DataEntry &entry = entries[index];

            if (entry.timestamp == DateTime::MinValue())
                continue; // skip empty

            if (finder(entry))
            {
                callback(entry);
                return true;
            }
        }

        return false;
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
    Mutex mutex;

    DataEntry entries[MAX_ENTRIES] = {};
    size_t writePtr = 0; // Always points to empty
};
