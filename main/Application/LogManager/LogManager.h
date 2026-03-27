#pragma once

#include "ServiceProvider.h"
#include "InitState.h"
#include "LogDefs.h"
#include "Mutex.h"
#include "EspFlash.h"
#include "flash_log.h"
#include "DateTime.h"
#include "esp_timer.h"

class TimeManager;

class LogManager {
    static constexpr const char* TAG = "LogManager";
    static constexpr const char* PARTITION_LABEL = "logdata";
    static constexpr size_t KEY_SIZE = sizeof(uint8_t);
    static constexpr size_t VALUE_SIZE = sizeof(uint32_t);
    static constexpr size_t MAX_BROADCAST_FIELDS = 8;
    static constexpr size_t MAX_PENDING_ENTRIES = 32;

public:
    explicit LogManager(ServiceProvider& serviceProvider);

    LogManager(const LogManager&) = delete;
    LogManager& operator=(const LogManager&) = delete;

    void Init();

    struct FieldPair { uint8_t key; uint32_t value; };

    using BroadcastFunc = void (*)(const char* json, int32_t len, void* ctx);
    void SetBroadcastCallback(BroadcastFunc func, void* ctx);

    /// Append a log entry with variadic key-value pairs. Thread-safe.
    /// Supports uint32_t, float, and DateTime values.
    /// If time is not yet synced, entries are buffered in RAM and flushed
    /// with corrected timestamps once SNTP completes.
    template<typename V, typename... Args>
    bool Append(LogKeys key, V value, Args... rest)
    {
        LOCK(mutex_);

        if (!timeSynced_)
        {
            return BufferPending(key, value, rest...);
        }

        broadcastFieldCount_ = 0;
        if (!log_.beginEntry()) return false;
        bool ok = AppendFields(key, value, rest...);
        if (!ok) { log_.finishEntry(); return false; }
        if (!log_.finishEntry()) return false;
        BroadcastLastEntry();
        return true;
    }

    /// Called when time becomes available. Flushes buffered entries.
    void OnTimeSynced();

    /// RAII view that holds the mutex and exposes iterators.
    class ReadView {
    public:
        ReadView(const FlashLog& log, const Mutex& mutex)
            : log_(log), mutex_(mutex) { mutex_.Take(); }
        ~ReadView() { mutex_.Give(); }

        ReadView(const ReadView&) = delete;
        ReadView& operator=(const ReadView&) = delete;
        ReadView(ReadView&&) = default;

        EntryIterator begin() const { return log_.begin(); }
        EntryIterator end()   const { return log_.end(); }

    private:
        const FlashLog& log_;
        const Mutex& mutex_;
    };

    ReadView Read() const { return ReadView(log_, mutex_); }

    uint32_t EntryCount() const;
    bool Erase();

private:
    ServiceProvider& serviceProvider_;
    InitState initState_;
    mutable Mutex mutex_;
    EspFlash flash_;
    FlashLog log_{flash_};
    bool timeSynced_ = false;

    BroadcastFunc broadcastFunc_ = nullptr;
    void* broadcastCtx_ = nullptr;

    // Temp buffer for broadcast
    FieldPair broadcastFields_[MAX_BROADCAST_FIELDS] = {};
    size_t broadcastFieldCount_ = 0;

    // Pending entries buffered before time sync
    struct PendingEntry {
        FieldPair fields[MAX_BROADCAST_FIELDS];
        size_t fieldCount = 0;
        int64_t uptimeUs = 0;  // esp_timer_get_time() at creation
    };
    PendingEntry pendingEntries_[MAX_PENDING_ENTRIES] = {};
    size_t pendingCount_ = 0;

    void BroadcastLastEntry();
    void FlushPending();

    template<typename V, typename... Args>
    bool BufferPending(LogKeys key, V value, Args... rest)
    {
        if (pendingCount_ >= MAX_PENDING_ENTRIES) return false;
        auto& entry = pendingEntries_[pendingCount_];
        entry.fieldCount = 0;
        entry.uptimeUs = esp_timer_get_time();
        CollectFields(entry, key, value, rest...);
        pendingCount_++;
        return true;
    }

    // Convert any value to uint32_t bits
    static uint32_t ToBits(uint32_t v) { return v; }
    static uint32_t ToBits(float v) { uint32_t b; memcpy(&b, &v, sizeof(b)); return b; }
    static uint32_t ToBits(DateTime v) { return static_cast<uint32_t>(v.UtcSeconds()); }

    template<typename V>
    void CollectFields(PendingEntry& entry, LogKeys key, V value)
    {
        if (entry.fieldCount < MAX_BROADCAST_FIELDS)
            entry.fields[entry.fieldCount++] = {static_cast<uint8_t>(key), ToBits(value)};
    }

    template<typename V, typename... Args>
    void CollectFields(PendingEntry& entry, LogKeys key, V value, Args... rest)
    {
        CollectFields(entry, key, value);
        CollectFields(entry, rest...);
    }

    template<typename V>
    bool AppendFields(LogKeys key, V value)
    {
        auto k = static_cast<uint8_t>(key);
        uint32_t bits = ToBits(value);
        if (broadcastFieldCount_ < MAX_BROADCAST_FIELDS)
            broadcastFields_[broadcastFieldCount_++] = {k, bits};
        return log_.field(&k, &bits, sizeof(bits));
    }

    template<typename V, typename... Args>
    bool AppendFields(LogKeys key, V value, Args... rest)
    {
        if (!AppendFields(key, value)) return false;
        return AppendFields(rest...);
    }
};
