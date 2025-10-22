#include "SensorManager.h"
#include "esp_check.h"
#include "Mutex.h"

SensorManager::SensorManager(ServiceProvider &ctx)
    : settingsManager(ctx.GetSettingsManager())
    , dataManager(ctx.GetDataManager())
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
    TriggerTemperatureConversions();
    lastTemperatureRead = NowMs();      // This ensures enough time for the first conversion
    lastBusScan = NowMs();
}

void SensorManager::Tick(TickContext &ctx)
{
    REQUIRE_READY(initGuard);
    LOCK(mutex);

    bool updateTemperatures = ctx.ElapsedAndReset(lastTemperatureRead, TEMPERATURE_READ_INTERVAL);

    if(updateTemperatures)
    {
        // Assume previous tick triggered conversions
        ReadTemperatures();
    }

    if (ctx.ElapsedAndReset(lastBusScan, BUS_SCAN_INTERVAL))
    {
        ScanBus();
    }

    if (updateTemperatures)
    {
        TriggerTemperatureConversions();
    }
}

float SensorManager::GetTemperature(int index)
{
    REQUIRE_READY(initGuard);
    LOCK(mutex);
    if(index < 0 || index >= sensorCount)
        return 0.0f;
    return sensors[index].temperatureC;
}

uint64_t SensorManager::GetAddress(int index)
{
    REQUIRE_READY(initGuard);
    LOCK(mutex);
    if(index < 0 || index >= sensorCount)
        return 0;
    return sensors[index].address;
}

int SensorManager::GetSensorCount()
{
    REQUIRE_READY(initGuard);
    LOCK(mutex);
    return sensorCount;
}

void SensorManager::ScanBus()
{
    REQUIRE_READY(initGuard);
    LOCK(mutex);

    ESP_LOGI(TAG, "Rescanning OneWire bus for sensors...");
    onewire_device_iter_handle_t iter = nullptr;
    ESP_ERROR_CHECK(onewire_new_device_iter(bus, &iter));

    onewire_device_t device;
    sensorCount = 0;

    while (onewire_device_iter_get_next(iter, &device) == ESP_OK && sensorCount < MAX_SENSORS)
    {
        ds18b20_config_t ds_cfg = {};
        if (ds18b20_new_device_from_enumeration(&device, &ds_cfg, &sensors[sensorCount].handle) == ESP_OK)
        {
            onewire_device_address_t addr;
            ds18b20_get_device_address(sensors[sensorCount].handle, &addr);
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

void SensorManager::TriggerTemperatureConversions()
{
    esp_err_t err = ds18b20_trigger_temperature_conversion_for_all(bus);
    if (err != ESP_OK)
        ESP_LOGE(TAG, "Failed to trigger temperature conversion: %s", esp_err_to_name(err));
}

void SensorManager::ReadTemperatures()
{
    for (int index = 0; index < sensorCount; ++index)
    {
        esp_err_t err = ds18b20_get_temperature(sensors[index].handle, &sensors[index].temperatureC);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to read DS18B20[%d]: %s", index, esp_err_to_name(err));
            continue;
        }

        err = ds18b20_get_device_address(sensors[index].handle, &sensors[index].address);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to read DS18B20[%d] address: %s", index, esp_err_to_name(err));
            continue;
        }

        DataEntry entry;
        entry.timestamp = DateTime::Now();
        entry.pairs[0] = { DataKey::MacAddress, (uint64_t)sensors[index].address};
        entry.pairs[1] = { DataKey::Temperature, sensors[index].temperatureC };
        dataManager.Append(entry);
    }
}
