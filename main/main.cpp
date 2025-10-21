#include <stdio.h>
#include "esp_log.h"
#include "AppContext.h"
#include "NvsStorage.h"
#include "InfluxClient.h"

constexpr const char *TAG = "Main";

AppContext appContext;

extern "C" void app_main(void)
{
    esp_log_level_set("esp-x509-crt-bundle", ESP_LOG_WARN);
    
    // Initialize NVS
    ESP_LOGI(TAG, "Initializing NVS...");
    NvsStorage::InitNvsPartition("nvs");
    NvsStorage::PrintStats("nvs");

    // Initialize application services
    appContext.GetSettingsManager().Init();
    appContext.GetWifiManager().Init();
    appContext.GetTimeManager().Init();
    appContext.GetSensorManager().Init();
    appContext.GetInfluxManager().Init();

    // --------------------------------------------------------
    // Main loop
    // --------------------------------------------------------
    ESP_LOGI(TAG, "Entering main loop...");
    while (true)
    {
        appContext.GetWifiManager().Loop();

        if(appContext.GetTimeManager().IsTimeValid())
        {
            float temp;
            if(appContext.GetSensorManager().GetTemperature(0, temp))
            {
                DateTime now = DateTime::Now();
                appContext.GetInfluxManager().Write("temperature", temp, now, pdMS_TO_TICKS(2000));
            }
            else
            {
                ESP_LOGW(TAG, "No temperature sensor available");
            }
            
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
