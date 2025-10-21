#include <stdio.h>
#include "esp_log.h"
#include "AppContext.h"
#include "NvsStorage.h"
#include "InfluxClient.h"

constexpr const char *TAG = "Main";

AppContext appContext;

extern "C" void app_main(void)
{
    // Initialize NVS
    ESP_LOGI(TAG, "Initializing NVS...");
    NvsStorage::InitNvsPartition("nvs");
    NvsStorage::PrintStats("nvs");

    // Initialize settings
    ESP_LOGI(TAG, "Initializing SettingsManager...");
    appContext.GetSettingsManager().Init();

    appContext.GetSettingsManager().Access([](RootSettings &settings) {
        ESP_LOGI(TAG, "Current settings:");

        appContext.GetSettingsManager().Print(settings);
    });

    // Initialize Wi-Fi (auto-connect)
    ESP_LOGI(TAG, "Initializing WifiManager...");
    appContext.GetWifiManager().Init();

    appContext.GetSensorManager().Init();

    // --------------------------------------------------------
    // Main loop
    // --------------------------------------------------------
    ESP_LOGI(TAG, "Entering main loop...");
    while (true)
    {
        appContext.GetWifiManager().Loop();
        appContext.GetSensorManager().Loop();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
