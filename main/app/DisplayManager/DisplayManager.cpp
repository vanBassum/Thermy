#include "DisplayManager.h"
#include "WifiManager.h"
#include "esp_log.h"
#include "esp_netif.h"

DisplayManager::DisplayManager(ServiceProvider &ctx)
    : _ctx(ctx)
{
}

void DisplayManager::Init()
{
    if (initGuard.IsReady())
        return;

    LOCK(mutex);

    driver.Init();
    initGuard.SetReady();
    ESP_LOGI(TAG, "DisplayManager initialized.");
}
