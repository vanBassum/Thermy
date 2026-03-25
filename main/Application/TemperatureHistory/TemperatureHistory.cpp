#include "TemperatureHistory.h"
#include "SensorManager/SensorManager.h"
#include "SettingsManager/SettingsManager.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include <algorithm>
#include <cassert>

TemperatureHistory::TemperatureHistory(ServiceProvider &ctx)
    : sensorManager(ctx.getSensorManager())
    , settingsManager(ctx.getSettingsManager())
{
}

void TemperatureHistory::Init()
{
    auto init = initState.TryBeginInit();
    if (!init)
        return;

    // Allocate buffer in PSRAM (falls back to internal RAM if no PSRAM)
    buffer = static_cast<TemperatureSample *>(
        heap_caps_calloc(MAX_SAMPLES, sizeof(TemperatureSample), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (!buffer)
    {
        ESP_LOGW(TAG, "PSRAM alloc failed, falling back to internal RAM");
        buffer = static_cast<TemperatureSample *>(calloc(MAX_SAMPLES, sizeof(TemperatureSample)));
    }
    assert(buffer && "Failed to allocate temperature history buffer");

    ESP_LOGI(TAG, "Buffer allocated: %d samples (%d KB) in %s",
             MAX_SAMPLES, (MAX_SAMPLES * sizeof(TemperatureSample)) / 1024,
             heap_caps_get_free_size(MALLOC_CAP_SPIRAM) > 0 ? "PSRAM" : "internal");

    int32_t rateSec = settingsManager.getInt("history.rate", DEFAULT_RATE_SECONDS);
    if (rateSec < 1)
        rateSec = 1;

    ESP_LOGI(TAG, "Sample rate: %lds, buffer: %d samples (%.1f hours max)",
             rateSec, MAX_SAMPLES, (float)(MAX_SAMPLES * rateSec) / 3600.0f);

    sampleTimer.Init("TempSample", pdMS_TO_TICKS(rateSec * 1000), true);
    sampleTimer.SetHandler([this]() { TakeSample(); });
    sampleTimer.Start();

    init.SetReady();
    ESP_LOGI(TAG, "Initialized");
}

void TemperatureHistory::TakeSample()
{
    TemperatureSample sample = {};

    for (int i = 0; i < 4; i++)
    {
        if (sensorManager.IsSlotActive(i))
            sample.temperatures[i] = sensorManager.GetTemperature(i);
        else
            sample.temperatures[i] = NAN;
    }

    LOCK(mutex);
    buffer[head] = sample;
    head = (head + 1) % MAX_SAMPLES;
    if (count < MAX_SAMPLES)
        count++;
}

size_t TemperatureHistory::GetSamples(TemperatureSample *out, size_t maxCount)
{
    LOCK(mutex);

    size_t n = std::min(maxCount, count);
    if (n == 0)
        return 0;

    size_t start = (head + MAX_SAMPLES - count) % MAX_SAMPLES;
    size_t offset = count - n;
    size_t readPos = (start + offset) % MAX_SAMPLES;

    for (size_t i = 0; i < n; i++)
    {
        out[i] = buffer[readPos];
        readPos = (readPos + 1) % MAX_SAMPLES;
    }

    return n;
}

size_t TemperatureHistory::GetCount()
{
    LOCK(mutex);
    return count;
}
