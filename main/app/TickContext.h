#pragma once
#include <stdint.h>
#include <esp_timer.h>

class TickContext
{
public:
    explicit TickContext(uint64_t nowMs, uint32_t defaultTickIntervalMs)
        : _timeSinceBootMs(nowMs),
          _tickIntervalMs(defaultTickIntervalMs)
    {
    }

    // ---- Accessors ----
    inline uint64_t TimeSinceBootMs() const { return _timeSinceBootMs; }
    inline uint32_t TickIntervalMs() const { return _tickIntervalMs; }
    inline bool PreventSleepRequested() const { return _preventSleep; }

    // ---- Utility: Safe time checks ----
    inline bool ElapsedAndReset(uint64_t &timerVar, uint64_t intervalMs) const
    {
        if ((uint64_t)(_timeSinceBootMs - timerVar) >= intervalMs)
        {
            timerVar = _timeSinceBootMs;
            return true;
        }
        return false;
    }

    inline bool HasElapsed(uint64_t startTime, uint64_t intervalMs) const
    {
        return (uint64_t)(_timeSinceBootMs - startTime) >= intervalMs;
    }

    // ---- Tick control ----
    inline void PreventSleep() { _preventSleep = true; }

    // Request a shorter tick interval (e.g. faster tick)
    inline void RequestFasterTick(uint32_t intervalMs)
    {
        if (intervalMs < _tickIntervalMs)
            _tickIntervalMs = intervalMs;
    }

    // Current timestamp helper
    static inline uint64_t NowMs()
    {
        return esp_timer_get_time() / 1000ULL;
    }

private:
    const uint64_t _timeSinceBootMs;
    uint32_t _tickIntervalMs;
    bool _preventSleep = false;
};
