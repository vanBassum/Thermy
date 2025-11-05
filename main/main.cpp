#include "AppContext.h"
#include "NvsStorage.h"
#include "FatFsDriver.h"
#include "esp_log.h"

constexpr const char *TAG = "Main";

AppContext appContext;

extern "C" void app_main(void)
{
    esp_log_level_set("esp-x509-crt-bundle", ESP_LOG_WARN);
    esp_log_level_set("pm", ESP_LOG_DEBUG);
    esp_log_level_set("wifi", ESP_LOG_WARN);

    ESP_LOGI(TAG, "Initializing NVS...");
    NvsStorage::InitNvsPartition("nvs");
    NvsStorage::PrintStats("nvs");

    ESP_LOGI(TAG, "Initializing FAT...");
    FatfsDriver fatFsDriver{"/fat", "fat"};
    fatFsDriver.Init();

    ESP_LOGI(TAG, "Starting managers...");
    appContext.Init();

    ESP_LOGI(TAG, "Initialization complete. Deleting main task...");
    vTaskDelete(nullptr);  // Delete this startup task
}
