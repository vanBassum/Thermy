#include <stdio.h>
#include "esp_log.h"
#include "AppContext.h"
#include "NvsStorage.h"
#include "InfluxClient.h"
#include "core_utils.h"
#include "esp_timer.h"
#include "esp_pm.h"
#include "esp_wifi.h"

constexpr const char *TAG = "Main";
AppContext appContext;

extern "C" void app_main(void)
{
    esp_log_level_set("esp-x509-crt-bundle", ESP_LOG_WARN);
    esp_log_level_set("pm", ESP_LOG_DEBUG);

    ESP_LOGI(TAG, "Initializing NVS...");
    NvsStorage::InitNvsPartition("nvs");
    NvsStorage::PrintStats("nvs");

    appContext.Init();

    const Milliseconds defaultTickInterval = Millis(1000);

    uint64_t totalSleepTime = 0;
    uint32_t tickCount = 0;

    while (true)
    {
        Milliseconds start = NowMs();

        TickContext tickCtx(start, defaultTickInterval);
        appContext.Tick(tickCtx);

        Milliseconds end = NowMs();
        Milliseconds elapsed = end - start;

        if (elapsed > tickCtx.TickInterval())
        {
            ESP_LOGW(TAG, "Tick overrun: took %llu ms, interval %llu ms",
                     elapsed, tickCtx.TickInterval());
        }
        else if (tickCtx.PreventSleepRequested())
        {
            Milliseconds remaining = tickCtx.TickInterval() - elapsed;
            vTaskDelay(pdMS_TO_TICKS(remaining)); // Just wait but stay awake
        }
        else
        {
            Milliseconds remaining = tickCtx.TickInterval() - elapsed;
            totalSleepTime += remaining;
            vTaskDelay(pdMS_TO_TICKS(remaining)); // later replace with deep sleep
            // Before we can do deep sleep, we need to move the datamanager to NVS storage (should beable to fit FLASH requirements okay)
            // We can opt for light sleep for now to save some power
        }

        tickCount++;
        if (tickCount >= 30) // every ~30 ticks (~30 seconds)
        {
            Milliseconds totalRuntimeMs = NowMs();
            float sleepPct = (totalRuntimeMs > 0)
                                 ? (100.0f * totalSleepTime / totalRuntimeMs)
                                 : 0.0f;
            float awakePct = 100.0f - sleepPct;

            // Convert to TimeSpan (assuming constructor uses seconds)
            TimeSpan totalRuntime = TimeSpan::FromSeconds(totalRuntimeMs / 1000);
            TimeSpan sleepTime = TimeSpan::FromSeconds(totalSleepTime / 1000);
            TimeSpan awakeTime = TimeSpan::FromSeconds((totalRuntimeMs - totalSleepTime) / 1000);

            char runtimeStr[32];
            char sleepStr[32];
            char awakeStr[32];

            snprintf(runtimeStr, sizeof(runtimeStr), "%02d:%02d:%02d",
                     totalRuntime.Hours(), totalRuntime.Minutes(), totalRuntime.Seconds());

            snprintf(sleepStr, sizeof(sleepStr), "%02d:%02d:%02d",
                        sleepTime.Hours(), sleepTime.Minutes(), sleepTime.Seconds());

            snprintf(awakeStr, sizeof(awakeStr), "%02d:%02d:%02d",
                        awakeTime.Hours(), awakeTime.Minutes(), awakeTime.Seconds());

            ESP_LOGI(TAG, "-----------------------------------------------");
            ESP_LOGI(TAG, " Runtime  | Awake    | Sleep    ");
            ESP_LOGI(TAG, " %s | %s | %s ", runtimeStr, awakeStr, sleepStr);
            ESP_LOGI(TAG, " 100%%     | %.1f%%     | %.1f%%   ", awakePct, sleepPct);
            ESP_LOGI(TAG, "-----------------------------------------------");
            tickCount = 0;
        }
    }
}
