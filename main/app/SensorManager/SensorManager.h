#pragma once
#include "ServiceProvider.h"
#include "rtos.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "onewire_bus.h"
#include "ds18b20.h"


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
    inline static constexpr TickType_t BUS_SCAN_INTERVAL            = pdMS_TO_TICKS(30000); 
    inline static constexpr TickType_t TEMPERATURE_READ_INTERVAL    = pdMS_TO_TICKS(1000); 

public:
    explicit SensorManager(ServiceProvider &ctx);

    void Init();
    float GetTemperature(int index);
    uint64_t GetAddress(int index);
    int GetSensorCount();

private:
    InitGuard initGuard;
    RecursiveMutex mutex;
    Task task;

    void Work();    
    void ScanBus();
    bool TriggerTemperatureConversions();
    bool ReadTemperatures();

    gpio_num_t one_wire_gpio = GPIO_NUM_4;
    onewire_bus_handle_t bus = nullptr;
    SensorContext sensors[MAX_SENSORS]{};
    int sensorCount = 0;
};
