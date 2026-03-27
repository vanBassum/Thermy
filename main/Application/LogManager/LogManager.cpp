#include "LogManager.h"
#include "JsonWriter.h"
#include "BufferStream.h"
#include "esp_log.h"
#include <cstdio>

LogManager::LogManager(ServiceProvider& serviceProvider)
    : serviceProvider_(serviceProvider)
{
}

void LogManager::Init()
{
    auto initAttempt = initState_.TryBeginInit();
    if (!initAttempt)
    {
        return;
    }

    if (!flash_.mount(PARTITION_LABEL))
    {
        ESP_LOGE(TAG, "Failed to mount flash partition");
        return;
    }

    if (!log_.init())
    {
        ESP_LOGI(TAG, "No valid log found, formatting");
        if (!log_.format(KEY_SIZE, VALUE_SIZE))
        {
            ESP_LOGE(TAG, "Format failed");
            return;
        }
        if (!log_.init())
        {
            ESP_LOGE(TAG, "Init failed after format");
            return;
        }
    }

    initAttempt.SetReady();
    ESP_LOGI(TAG, "Initialized (%lu entries on flash)", (unsigned long)log_.entryCount());
}

void LogManager::SetBroadcastCallback(BroadcastFunc func, void* ctx)
{
    broadcastFunc_ = func;
    broadcastCtx_ = ctx;
}

uint32_t LogManager::EntryCount() const
{
    LOCK(mutex_);
    return log_.entryCount();
}

bool LogManager::Erase()
{
    LOCK(mutex_);
    if (!log_.format(KEY_SIZE, VALUE_SIZE)) return false;
    return log_.init();
}

void LogManager::BroadcastLastEntry()
{
    if (!broadcastFunc_ || broadcastFieldCount_ == 0) return;

    char buf[256];
    BufferStream stream(buf, sizeof(buf));
    JsonWriter json(stream);

    json.beginObject();
    json.fieldArray("logEntry");
    for (size_t i = 0; i < broadcastFieldCount_; i++)
    {
        json.beginArray();
        json.value(static_cast<int32_t>(broadcastFields_[i].key));
        json.value(static_cast<int32_t>(broadcastFields_[i].value));
        json.endArray();
    }
    json.endArray();
    json.endObject();

    broadcastFunc_(stream.data(), stream.length(), broadcastCtx_);
}

void LogManager::OnTimeSynced()
{
    LOCK(mutex_);
    if (timeSynced_) return;
    timeSynced_ = true;

    if (pendingCount_ > 0)
    {
        ESP_LOGI(TAG, "Time synced, flushing %u pending entries", (unsigned)pendingCount_);
        FlushPending();
    }
}

void LogManager::FlushPending()
{
    // Current real time and uptime — used to back-calculate real timestamps
    int64_t nowUs = esp_timer_get_time();
    uint32_t nowUtc = static_cast<uint32_t>(DateTime::Now().UtcSeconds());

    for (size_t i = 0; i < pendingCount_; i++)
    {
        auto& entry = pendingEntries_[i];

        // Calculate real timestamp: now - (uptimeNow - uptimeThen)
        int64_t ageUs = nowUs - entry.uptimeUs;
        uint32_t ageSec = static_cast<uint32_t>(ageUs / 1000000);
        uint32_t realTimestamp = (nowUtc > ageSec) ? nowUtc - ageSec : 0;

        broadcastFieldCount_ = 0;
        if (!log_.beginEntry()) continue;

        bool ok = true;
        for (size_t f = 0; f < entry.fieldCount && ok; f++)
        {
            uint8_t key = entry.fields[f].key;
            uint32_t value = entry.fields[f].value;

            // Replace placeholder timestamp with real one
            if (static_cast<LogKeys>(key) == LogKeys::TimeStamp)
                value = realTimestamp;

            if (broadcastFieldCount_ < MAX_BROADCAST_FIELDS)
                broadcastFields_[broadcastFieldCount_++] = {key, value};

            ok = log_.field(&key, &value, sizeof(value));
        }

        if (!ok) { log_.finishEntry(); continue; }
        log_.finishEntry();
    }

    ESP_LOGI(TAG, "Flushed %u entries, total now %lu",
             (unsigned)pendingCount_, (unsigned long)log_.entryCount());
    pendingCount_ = 0;
}
