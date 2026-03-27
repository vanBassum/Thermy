#include <stdio.h>
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_app_desc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ApplicationContext.h"
#include "LogDefs.h"
#include "DateTime.h"

static const char* TAG = "main";

ApplicationContext g_appContext;

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Starting up...");
    g_appContext.getConsoleManager().Init();
    g_appContext.getLogManager().Init();
    g_appContext.getSettingsManager().Init();
    g_appContext.getNetworkManager().Init();
    g_appContext.getTimeManager().Init();
    g_appContext.getSensorManager().Init();
    g_appContext.getMonitorManager().Init();
    g_appContext.getDisplayManager().Init();
    g_appContext.getCommandManager().Init();
    g_appContext.getMqttManager().Init();
    g_appContext.getDeviceManager().Init();
    g_appContext.getHomeAssistantManager().Init();
    g_appContext.getUpdateManager().Init();
    g_appContext.getWebServerManager().Init();

    // Log boot event with firmware version
    const esp_app_desc_t* app = esp_app_get_description();
    unsigned major = 0, minor = 0, patch = 0;
    sscanf(app->version, "%u.%u.%u", &major, &minor, &patch);
    uint32_t versionPacked = (major << 16) | (minor << 8) | patch;
    g_appContext.getLogManager().Append(
        LogKeys::TimeStamp, DateTime::Now(),
        LogKeys::LogCode, static_cast<uint32_t>(LogCode::SystemBoot),
        LogKeys::FirmwareVersion, versionPacked);

    // Mark firmware as valid so the bootloader doesn't roll back on next reboot
    esp_ota_mark_app_valid_cancel_rollback();
    ESP_LOGI(TAG, "All managers initialized, firmware confirmed valid");
}
