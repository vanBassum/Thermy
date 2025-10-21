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

    // Initialize Display
    ESP_LOGI(TAG, "Initializing DisplayManager...");
    appContext.GetDisplayManager().Init();


    InfluxClient influx(
        "http://192.168.50.96:8086/api/v2/write",
        "TIPK7M91CfTF7mgC0BsV26M-VVXVn2RgQiA8yOJxUNAwp-G-a40MkOP4rP1c4ke5RPYMP5F9SPCEC2pomwLfJA==",
        "koole",
        "thermy");

        influx
            .Measurement("temperature", 1761047733)
            .withTag("sensor", "outdoor")
            .withField("value", 23.4f)
            .withMeasurement("humidity", 1761047733)
            .withTag("sensor", "outdoor")
            .withField("value", 50.3f)
            .Finish();


    // --------------------------------------------------------
    // Main loop
    // --------------------------------------------------------
    ESP_LOGI(TAG, "Entering main loop...");
    while (true)
    {
        appContext.GetWifiManager().Loop();
        appContext.GetDisplayManager().Loop();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
