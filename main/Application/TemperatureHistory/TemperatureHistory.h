#pragma once
#include "ServiceProvider.h"
#include "InitState.h"
#include "Mutex.h"
#include "Timer.h"
#include <ctime>

class SensorManager;
class SettingsManager;

struct TemperatureSample
{
    time_t timestamp = 0;
    float temperatures[4] = {};
    uint8_t activeMask = 0; // bit per slot

    bool IsActive(int slot) const { return (activeMask >> slot) & 1; }
};

class TemperatureHistory
{
    inline static constexpr const char *TAG = "TempHistory";

public:
    static constexpr size_t MAX_SAMPLES = 2048;
    static constexpr int32_t DEFAULT_RATE_SECONDS = 10;

    explicit TemperatureHistory(ServiceProvider &ctx);

    void Init();

    /// Copy up to maxCount most recent samples into out (oldest first).
    /// Returns number of samples written.
    size_t GetSamples(TemperatureSample *out, size_t maxCount);

    /// Current number of stored samples.
    size_t GetCount();

    /// Max capacity.
    static constexpr size_t GetMaxSamples() { return MAX_SAMPLES; }

private:
    SensorManager &sensorManager;
    SettingsManager &settingsManager;
    InitState initState;
    Mutex mutex;
    Timer sampleTimer;

    TemperatureSample buffer[MAX_SAMPLES];
    size_t head = 0;  // next write position
    size_t count = 0;

    void TakeSample();
    void ReconfigureTimer();
};
