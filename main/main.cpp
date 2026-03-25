#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ApplicationContext.h"

static const char* TAG = "main";

ApplicationContext g_appContext;

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Starting up...");
    g_appContext.getLogManager().Init();
    g_appContext.getSettingsManager().Init();
    g_appContext.getDisplayManager().Init();
    g_appContext.getNetworkManager().Init();
    g_appContext.getTimeManager().Init();
    g_appContext.getSensorManager().Init();
    g_appContext.getCommandManager().Init();
    g_appContext.getUpdateManager().Init();
    g_appContext.getWebServerManager().Init();
}
