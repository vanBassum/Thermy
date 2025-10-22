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

class SensorManager
{
    inline static constexpr const char *TAG = "SensorManager";
    inline static constexpr size_t MAX_SENSORS = 4;

public:
    explicit SensorManager(ServiceProvider &ctx);

    void Init();
    void Tick(TickContext& ctx) {}

    void ScanBus();
    int GetSensorCount() const { return sensorCount; }
    bool StartReading(int index);
    bool GetAddress(int index, uint8_t outAddress[8]);
    bool GetTemperature(int index, float &outTemp);

private:
    SettingsManager &settingsManager;
    InitGuard initGuard;
    RecursiveMutex mutex;

    onewire_bus_handle_t bus = nullptr;
    ds18b20_device_handle_t sensors[MAX_SENSORS]{};
    int sensorCount = 0;
    gpio_num_t one_wire_gpio = GPIO_NUM_2;
};
