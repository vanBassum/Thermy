#pragma once
#include <stdint.h>
#include <esp_timer.h>

typedef uint64_t Milliseconds;

// ---- Factory helpers ----
inline static constexpr Milliseconds Millis(uint64_t val)
{
    return val;
}

static inline Milliseconds NowMs()
{
    return esp_timer_get_time() / 1000ULL;
}

class TickContext
{
public:
    explicit TickContext(Milliseconds now, Milliseconds defaultTickIntervalMs)
        : _timeSinceBootMs(now),
          _tickIntervalMs(defaultTickIntervalMs)
    {
    }

    // ---- Accessors ----
    inline Milliseconds TimeSinceBoot() const { return _timeSinceBootMs; }
    inline Milliseconds TickInterval() const { return _tickIntervalMs; }
    inline bool PreventSleepRequested() const { return _preventSleep; }

    // ---- Safe time checks ----
    inline bool ElapsedAndReset(Milliseconds &timerVar, Milliseconds interval) const
    {
        if ((_timeSinceBootMs - timerVar) >= interval)
        {
            timerVar = _timeSinceBootMs;
            return true;
        }
        return false;
    }

    inline bool HasElapsed(Milliseconds startTime, Milliseconds interval) const
    {
        return (_timeSinceBootMs - startTime) >= interval;
    }

    // ---- Tick control ----
    inline void PreventSleep() { _preventSleep = true; }

    inline void RequestFasterTick(Milliseconds interval)
    {
        if (interval < _tickIntervalMs)
            _tickIntervalMs = interval;
    }

    static inline Milliseconds NowMs() { return ::NowMs(); }

private:
    const Milliseconds _timeSinceBootMs;
    Milliseconds _tickIntervalMs;
    bool _preventSleep = false;
};

