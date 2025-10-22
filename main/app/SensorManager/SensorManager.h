#pragma once
#include "ServiceProvider.h"
#include "InitGuard.h"
#include "RecursiveMutex.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "onewire_bus.h"
#include "ds18b20.h"
#include "SettingsManager.h"
#include "TickContext.h"
#include "DataManager.h"


struct SensorContext
{
    ds18b20_device_handle_t handle = nullptr;
    onewire_device_address_t address = 0;
    float temperatureC = 0.0f;
};

class SensorManager
{
    inline static constexpr const char *TAG = "SensorManager";
    inline static constexpr size_t MAX_SENSORS = 4;
    inline static constexpr Milliseconds BUS_SCAN_INTERVAL = 10000;
    inline static constexpr Milliseconds TEMPERATURE_READ_INTERVAL = 1000;

public:
    explicit SensorManager(ServiceProvider &ctx);

    void Init();
    void Tick(TickContext& ctx);
    float GetTemperature(int index);
    uint64_t GetAddress(int index);
    int GetSensorCount();

private:
    SettingsManager &settingsManager;
    DataManager &dataManager;
    InitGuard initGuard;
    RecursiveMutex mutex;
    Milliseconds lastBusScan = 0;
    Milliseconds lastTemperatureRead = 0;

    void ScanBus();
    void TriggerTemperatureConversions();
    void ReadTemperatures();

    gpio_num_t one_wire_gpio = GPIO_NUM_2;
    onewire_bus_handle_t bus = nullptr;
    SensorContext sensors[MAX_SENSORS]{};
    int sensorCount = 0;
};
