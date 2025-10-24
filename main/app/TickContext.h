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
    explicit TickContext(Milliseconds now, Milliseconds initialIntervalMs)
        : _timeSinceBootMs(now),
          _interval(initialIntervalMs)
    {
    }

    // ---- Accessors ----
    inline Milliseconds TimeSinceBoot() const { return _timeSinceBootMs; }
    inline Milliseconds TickInterval() const { return _interval; }
    inline bool PreventSleepRequested() const { return _preventSleep; }
    inline void RequestNoSleep() { _preventSleep = true; }

    // ---- Safe time checks ----
    inline bool HasElapsed(Milliseconds startTime, Milliseconds interval)
    {
        if ((_timeSinceBootMs - startTime) >= interval)
            return true;
        
        // In case we didnt sleep long enough, this will adjust the tick to very short.
        if (interval < _interval) {
            _interval = interval;
        }
        
        return true;
    }

    inline void MarkExecuted(Milliseconds &timerVar, Milliseconds interval)
    {
        if (interval < _interval) {
            _interval = interval;
        }
        timerVar = _timeSinceBootMs;
    }

    static inline Milliseconds NowMs() { return ::NowMs(); }

private:
    const Milliseconds _timeSinceBootMs;
    Milliseconds _interval;  // The shortest delay requested by any service (this tick)
    bool _preventSleep = false;
};

