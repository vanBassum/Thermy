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

    const uint32_t defaultTickIntervalMs = 1000;
    uint32_t currentTickInterval = defaultTickIntervalMs;
    uint64_t lastTickTime = TickContext::NowMs();

    while (true)
    {
        uint64_t start = TickContext::NowMs();

        TickContext tickCtx(start, defaultTickIntervalMs);
        appContext.Tick(tickCtx);

        uint64_t end = TickContext::NowMs();
        uint64_t elapsed = end - start;

        if (elapsed > tickCtx.TickIntervalMs())
        {
            ESP_LOGW(TAG, "Tick overrun: took %llu ms (interval %u ms)",
                     elapsed, tickCtx.TickIntervalMs());
        }
        else if (!tickCtx.PreventSleepRequested())
        {
            uint32_t remaining = tickCtx.TickIntervalMs() - elapsed;
            vTaskDelay(pdMS_TO_TICKS(remaining));  // will later become deep sleep
        }

        currentTickInterval = tickCtx.TickIntervalMs();
        lastTickTime = TickContext::NowMs();
    }
}
