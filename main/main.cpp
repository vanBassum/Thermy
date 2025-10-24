#include <stdio.h>
#include "esp_log.h"
#include "AppContext.h"
#include "NvsStorage.h"
#include "InfluxClient.h"
#include "core_utils.h"
#include "esp_timer.h"
#include "esp_pm.h"
#include "esp_wifi.h"
#include "TickContext.h"
#include "SimpleStats.h"

constexpr const char *TAG = "Main";


// ---- MeasureTick helper ----
template <typename F>
Milliseconds MeasureTick(F &&func)
{
    Milliseconds start = NowMs();
    func();
    return NowMs() - start;
}

// ---- Stats array ----
static SimpleStats g_stats[] = {
    SimpleStats("Settings"),
    SimpleStats("Display"),
    SimpleStats("Wifi"),
    SimpleStats("Time"),
    SimpleStats("Sensor"),
    SimpleStats("Influx"),
    SimpleStats("Web"),
    SimpleStats("FTP"),
    SimpleStats("WebUpdate"),
    SimpleStats("Data"),
};

constexpr size_t NUM_STATS = sizeof(g_stats) / sizeof(g_stats[0]);

AppContext appContext(g_stats, NUM_STATS);

// ---- Tick all services and collect stats ----
void TickAllServices(TickContext &ctx, SimpleStats stats[])
{
    stats[0].AddValue(MeasureTick([&]() { appContext.GetSettingsManager().Tick(ctx); }));
    stats[1].AddValue(MeasureTick([&]() { appContext.GetDisplayManager().Tick(ctx); }));
    stats[2].AddValue(MeasureTick([&]() { appContext.GetWifiManager().Tick(ctx); }));
    stats[3].AddValue(MeasureTick([&]() { appContext.GetTimeManager().Tick(ctx); }));
    stats[4].AddValue(MeasureTick([&]() { appContext.GetSensorManager().Tick(ctx); }));
    stats[5].AddValue(MeasureTick([&]() { appContext.GetInfluxManager().Tick(ctx); }));
    stats[6].AddValue(MeasureTick([&]() { appContext.GetWebManager().Tick(ctx); }));
    stats[7].AddValue(MeasureTick([&]() { appContext.GetFtpManager().Tick(ctx); }));
    stats[8].AddValue(MeasureTick([&]() { appContext.GetWebUpdateManager().Tick(ctx); }));
    stats[9].AddValue(MeasureTick([&]() { appContext.GetDataManager().Tick(ctx); }));
}


// ---- Report all stats in a markdown-style table ----
void ReportStatistics(SimpleStats stats[], size_t count)
{
    static char buf[4096];
    char* bufPtr = buf;
    size_t remaining = sizeof(buf);

    // Write header
    int written = SimpleStats::PrintHeader(bufPtr, remaining);
    if (written < 0) return; // snprintf error
    bufPtr += written;
    remaining -= (written > 0) ? written : 0;

    // Write rows
    for (size_t i = 0; i < count && remaining > 0; ++i)
    {
        written = stats[i].PrintRow(bufPtr, remaining);
        if (written < 0) break; // snprintf error
        bufPtr += written;
        remaining -= (written > 0) ? written : 0;
    }

    // Ensure null termination
    *bufPtr = '\0';

    ESP_LOGI(TAG, "Runtimes:\n%s", buf);
}


extern "C" void app_main(void)
{
    esp_log_level_set("esp-x509-crt-bundle", ESP_LOG_WARN);
    esp_log_level_set("pm", ESP_LOG_DEBUG);

    ESP_LOGI(TAG, "Initializing NVS...");
    NvsStorage::InitNvsPartition("nvs");
    NvsStorage::PrintStats("nvs");

    appContext.Init();

    const Milliseconds defaultTickInterval = Millis(1000);

    while (true)
    {
        Milliseconds start = NowMs();
        TickContext ctx(start, defaultTickInterval);

        TickAllServices(ctx, g_stats);
        Milliseconds elapsed = NowMs() - start;

        Milliseconds interval = ctx.TickInterval();
        Milliseconds remaining = (elapsed < interval) ? (interval - elapsed) : portTICK_PERIOD_MS; // minimum delay

        if (elapsed > interval)
        {
            ESP_LOGW(TAG, "Tick overrun! elapsed=%llums interval=%llums", elapsed, interval);
            ReportStatistics(g_stats, NUM_STATS);
        }
        
        vTaskDelay(pdMS_TO_TICKS(remaining));
    }
}
