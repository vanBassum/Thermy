#include "SensorManager.h"
#include "esp_check.h"
#include "Mutex.h"

SensorManager::SensorManager(ServiceProvider &ctx)
    : settingsManager(ctx.GetSettingsManager())
{
}

void SensorManager::Init()
{
    if (initGuard.IsReady())
        return;

    LOCK(mutex);

    settingsManager.Access([&](RootSettings &settings)
    {
        one_wire_gpio = static_cast<gpio_num_t>(settings.system.oneWireGpio);
    });


    ESP_LOGI(TAG, "Initializing OneWire bus on GPIO%d", one_wire_gpio);

    onewire_bus_config_t bus_cfg = {
        .bus_gpio_num = one_wire_gpio,
        .flags = {.en_pull_up = 1}
    };
    onewire_bus_rmt_config_t rmt_cfg = {
        .max_rx_bytes = 10
    };

    ESP_ERROR_CHECK(onewire_new_bus_rmt(&bus_cfg, &rmt_cfg, &bus));

    initGuard.SetReady();
    ScanBus();
}

void SensorManager::ScanBus()
{
    REQUIRE_READY(initGuard);
    LOCK(mutex);

    onewire_device_iter_handle_t iter = nullptr;
    ESP_ERROR_CHECK(onewire_new_device_iter(bus, &iter));

    onewire_device_t device;
    sensorCount = 0;

    while (onewire_device_iter_get_next(iter, &device) == ESP_OK && sensorCount < MAX_SENSORS)
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


bool SensorManager::StartReading(int index)
{
    REQUIRE_READY(initGuard);
    LOCK(mutex);

    if (index < 0 || index >= sensorCount)
    {
        ESP_LOGW(TAG, "StartReading: invalid index %d", index);
        return false;
    }

    esp_err_t err = ds18b20_trigger_temperature_conversion_for_all(bus);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to trigger temperature conversion: %s", esp_err_to_name(err));
        return false;
    }

    return true;
}

bool SensorManager::GetAddress(int index, uint8_t outAddress[8])
{
    REQUIRE_READY(initGuard);
    LOCK(mutex);

    if (index < 0 || index >= sensorCount)
        return false;

    onewire_device_address_t addr;
    esp_err_t err = ds18b20_get_device_address(sensors[index], &addr);
    if (err == ESP_OK)
    {
        memcpy(outAddress, &addr, sizeof(addr));
        return true;
    }

    ESP_LOGE(TAG, "GetAddress failed for index %d: %s", index, esp_err_to_name(err));
    return false;
}

bool SensorManager::GetTemperature(int index, float &outTemp)
{
    REQUIRE_READY(initGuard);
    LOCK(mutex);

    if (index < 0 || index >= sensorCount)
        return false;

    // Wait for conversion to complete (DS18B20 typical max: 750ms)
    vTaskDelay(pdMS_TO_TICKS(750));

    esp_err_t err = ds18b20_get_temperature(sensors[index], &outTemp);
    if (err == ESP_OK)
    {
        onewire_device_address_t addr;
        ds18b20_get_device_address(sensors[index], &addr);
        ESP_LOGI(TAG, "DS18B20[%d] (%016llX): %.2f°C", index, addr, outTemp);
        return true;
    }

    ESP_LOGE(TAG, "Failed to read DS18B20[%d]: %s", index, esp_err_to_name(err));
    return false;
}