#include <stdio.h>
#include "esp_log.h"
#include "AppContext.h"
#include "NvsStorage.h"
#include "InfluxClient.h"
#include "core_utils.h"
#include "esp_timer.h"

constexpr const char *TAG = "Main";
AppContext appContext;

extern "C" void app_main(void)
{
    esp_log_level_set("esp-x509-crt-bundle", ESP_LOG_WARN);

    ESP_LOGI(TAG, "Initializing NVS...");
    NvsStorage::InitNvsPartition("nvs");
    NvsStorage::PrintStats("nvs");

    appContext.Init();

    const Milliseconds defaultTickInterval = Millis(1000);

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
        else if (!tickCtx.PreventSleepRequested())
        {
            Milliseconds remaining = tickCtx.TickInterval() - elapsed;
            vTaskDelay(pdMS_TO_TICKS(remaining));  // later replace with deep sleep
        }
    }
}
