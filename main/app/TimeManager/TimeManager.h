#pragma once
#include "ServiceProvider.h"
#include "InitGuard.h"
#include "RecursiveMutex.h"
#include "DateTime.h"
#include "TickContext.h"

class TimeManager
{
    inline static constexpr const char *TAG = "TimeManager";

public:
    explicit TimeManager(ServiceProvider &ctx);
    ~TimeManager();

    void Init();
    void Tick(TickContext& ctx) {}
    bool IsTimeValid() const;

    // Accessors
    DateTime GetSyncTime() const { return syncTime; }
    TimeSpan GetUptimeSinceFirstSync() const;
    bool HasSynced() const { return synced; }

private:
    ServiceProvider &_ctx;
    InitGuard initGuard;
    RecursiveMutex mutex;
    bool synced = false;
    uint32_t lastSyncAttemptMs = 0;

    // Reference point for time correlation
    uint64_t syncUptimeUs = 0;
    DateTime syncTime;

    static void TimeSyncCallback(struct timeval *tv);
};
