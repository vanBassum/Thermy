#pragma once
#include "ServiceProvider.h"
#include "InitGuard.h"
#include "RecursiveMutex.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "onewire_bus.h"
#include "ds18b20.h"

class SensorManager
{
    inline static constexpr const char *TAG = "SensorManager";

public:
    explicit SensorManager(ServiceProvider &ctx);

    void Init();

    int GetSensorCount() const { return sensorCount; }
    bool GetTemperature(int index, float &outTemp);

private:
    ServiceProvider &_ctx;
    InitGuard initGuard;
    RecursiveMutex mutex;

    onewire_bus_handle_t bus = nullptr;
    ds18b20_device_handle_t sensors[4]{};
    int sensorCount = 0;

    void ScanBus();
    void ReadTemperatures();

    static constexpr gpio_num_t ONEWIRE_GPIO = GPIO_NUM_4;
};
