#include "SensorManager.h"
#include "esp_check.h"
#include "Mutex.h"
#include <algorithm>
#include "core_utils.h"

SensorManager::SensorManager(ServiceProvider &ctx)
{
}

void SensorManager::Init()
{
    auto init = initState.TryBeginInit();
    if (!init)
        return;

    ESP_LOGI(TAG, "Initializing OneWire bus on GPIO%d", one_wire_gpio);

    onewire_bus_config_t bus_cfg = {
        .bus_gpio_num = one_wire_gpio,
        .flags = {.en_pull_up = 1}
    };
    onewire_bus_rmt_config_t rmt_cfg = {
        .max_rx_bytes = 10
    };

    ESP_ERROR_CHECK(onewire_new_bus_rmt(&bus_cfg, &rmt_cfg, &bus));

    task.Init("SensorManager", 6, 4096);
    task.SetHandler([this](){ Work(); });
    task.Run();

    init.SetReady();
}

float SensorManager::GetTemperature(int index)
{
    WAIT_FOR_READY(initState);
    LOCK(mutex);
    if(index < 0 || index >= sensorCount)
        return 0.0f;
    return sensors[index].temperatureC;
}

uint64_t SensorManager::GetAddress(int index)
{
    WAIT_FOR_READY(initState);
    LOCK(mutex);
    if(index < 0 || index >= sensorCount)
        return 0;
    return sensors[index].address;
}

int SensorManager::GetSensorCount()
{
    WAIT_FOR_READY(initState);
    LOCK(mutex);
    return sensorCount;
}

void SensorManager::Work()
{
    TickType_t lastBusScan = 0;
    TickType_t lastTemperatureRead = 0;

    ScanBus();
    TriggerTemperatureConversions();


    while(1)
    {
        TickType_t now = xTaskGetTickCount();

        bool success = true;

        if (IsElapsed(now, lastTemperatureRead, TEMPERATURE_READ_INTERVAL))
        {
            success &= ReadTemperatures();
            success &= TriggerTemperatureConversions();
            lastTemperatureRead = now;
        }

        if (IsElapsed(now, lastBusScan, BUS_SCAN_INTERVAL) || (!success))
        {
            ScanBus();
            lastBusScan = now;
        }

        TickType_t busScanSleep = GetSleepTime(now, lastBusScan, BUS_SCAN_INTERVAL);
        TickType_t tempReadSleep = GetSleepTime(now, lastTemperatureRead, TEMPERATURE_READ_INTERVAL);
        TickType_t sleepTime = std::min(busScanSleep, tempReadSleep);
        vTaskDelay(sleepTime);
    }
}

void SensorManager::ScanBus()
{
    LOCK(mutex);

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
            sensorCount++;
        }
        else
        {
            ESP_LOGW(TAG, "Found non-DS18B20 device: %016llX", device.address);
        }
    }
    onewire_del_device_iter(iter);
}

bool SensorManager::TriggerTemperatureConversions()
{
    LOCK(mutex);

    esp_err_t err;

    if(sensorCount == 0)
        return true; // No sensors to trigger

    // Reset the OneWire bus
    err = onewire_bus_reset(bus);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "OneWire reset failed: %s", esp_err_to_name(err));
        return false;
    }

    // Broadcast "Skip ROM" command (0xCC) — all sensors will listen
    err = onewire_bus_write_bytes(bus, (uint8_t[]){ 0xCC }, 1);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to send Skip ROM: %s", esp_err_to_name(err));
        return false;
    }

    // Send "Convert T" command (0x44) to start temperature conversion
    err = onewire_bus_write_bytes(bus, (uint8_t[]){ 0x44 }, 1);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to send Convert T command: %s", esp_err_to_name(err));
        return false;
    }

    // Done! The DS18B20 sensors now convert temperature asynchronously (~750 ms at 12-bit)
    return true;
}



bool SensorManager::ReadTemperatures()
{
    LOCK(mutex);
    bool success = true;

    for (int index = 0; index < sensorCount; ++index)
    {
        esp_err_t err = ds18b20_get_temperature(sensors[index].handle, &sensors[index].temperatureC);
        if (err != ESP_OK)
        {
            success = false;
            ESP_LOGE(TAG, "Failed to read DS18B20[%d]: %s", index, esp_err_to_name(err));
            continue;
        }

        err = ds18b20_get_device_address(sensors[index].handle, &sensors[index].address);
        if (err != ESP_OK)
        {
            success = false;
            ESP_LOGE(TAG, "Failed to read DS18B20[%d] address: %s", index, esp_err_to_name(err));
            continue;
        }
    }
    return success;
}
