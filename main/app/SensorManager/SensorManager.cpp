#include "SensorManager.h"
#include "esp_check.h"
#include "Mutex.h"

#define ONEWIRE_GPIO    4

SensorManager::SensorManager(ServiceProvider &ctx)
    : _ctx(ctx)
{
}

void SensorManager::Init()
{
    if (initGuard.IsReady())
        return;

    LOCK(mutex);

    ESP_LOGI(TAG, "Initializing OneWire bus on GPIO%d", ONEWIRE_GPIO);

    onewire_bus_config_t bus_cfg = {
        .bus_gpio_num = ONEWIRE_GPIO,
        .flags = {.en_pull_up = 1}
    };
    onewire_bus_rmt_config_t rmt_cfg = {
        .max_rx_bytes = 10
    };

    ESP_ERROR_CHECK(onewire_new_bus_rmt(&bus_cfg, &rmt_cfg, &bus));
    ScanBus();

    initGuard.SetReady();
}

void SensorManager::ScanBus()
{
    ESP_LOGI(TAG, "Scanning OneWire bus for DS18B20 sensors...");

    onewire_device_iter_handle_t iter = nullptr;
    ESP_ERROR_CHECK(onewire_new_device_iter(bus, &iter));

    onewire_device_t device;
    sensorCount = 0;

    while (onewire_device_iter_get_next(iter, &device) == ESP_OK && sensorCount < 4)
    {
        ds18b20_config_t ds_cfg = {};
        if (ds18b20_new_device_from_enumeration(&device, &ds_cfg, &sensors[sensorCount]) == ESP_OK)
        {
            onewire_device_address_t addr;
            ds18b20_get_device_address(sensors[sensorCount], &addr);
            ESP_LOGI(TAG, "Found DS18B20[%d] address: %016llX", sensorCount, addr);
            sensorCount++;
        }
        else
        {
            ESP_LOGW(TAG, "Found non-DS18B20 device: %016llX", device.address);
        }
    }

    onewire_del_device_iter(iter);
    ESP_LOGI(TAG, "Found %d DS18B20 sensor(s)", sensorCount);
}

bool SensorManager::GetTemperature(int index, float &outTemp)
{
    if (index < 0 || index >= sensorCount)
        return false;

    esp_err_t err = ds18b20_trigger_temperature_conversion_for_all(bus);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Temperature conversion trigger failed: %s", esp_err_to_name(err));
        return false;
    }

    vTaskDelay(pdMS_TO_TICKS(750)); // wait for conversion

    err = ds18b20_get_temperature(sensors[index], &outTemp);
    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "Temperature DS18B20[%d]: %.2f°C", index, outTemp);
        return true;
    }

    ESP_LOGE(TAG, "Failed to read DS18B20[%d]: %s", index, esp_err_to_name(err));
    return false;
}