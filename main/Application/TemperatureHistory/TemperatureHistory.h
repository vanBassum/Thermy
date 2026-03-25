#pragma once
#include "ServiceProvider.h"
#include "InitState.h"
#include "Mutex.h"
#include "Timer.h"
#include <cmath>

class SensorManager;
class SettingsManager;

struct TemperatureSample
{
    float temperatures[4] = {NAN, NAN, NAN, NAN};

    bool IsActive(int slot) const { return !std::isnan(temperatures[slot]); }
};

class TemperatureHistory
{
    inline static constexpr const char *TAG = "TempHistory";

public:
    static constexpr size_t MAX_SAMPLES = 8192;
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

    TemperatureSample *buffer = nullptr;
    size_t head = 0;
    size_t count = 0;

    void TakeSample();
};
