#pragma once
#include "ServiceProvider.h"
#include "rtos.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "onewire_bus.h"
#include "ds18b20.h"

class SettingsManager;

struct SensorSlot
{
    uint64_t configuredAddress = 0;   // from settings (0 = unassigned)
    ds18b20_device_handle_t handle = nullptr;
    float temperatureC = 0.0f;
    bool active = false;              // true if sensor found on bus
};

class SensorManager
{
    inline static constexpr const char *TAG = "SensorManager";
    static constexpr size_t MAX_SENSORS = 4;
    static constexpr TickType_t BUS_SCAN_INTERVAL         = pdMS_TO_TICKS(300000); // 5 minutes
    static constexpr TickType_t TEMPERATURE_READ_INTERVAL  = pdMS_TO_TICKS(1000);

public:
    explicit SensorManager(ServiceProvider &ctx);

    void Init();

    // Slot-based access (slot 0=Red, 1=Blue, 2=Green, 3=Yellow)
    float GetTemperature(int slot);
    uint64_t GetSlotAddress(int slot);
    bool IsSlotActive(int slot);

    // Pending sensor management (thread-safe, called from LVGL task)
    bool HasPendingSensor();
    uint64_t GetPendingSensorAddress();
    void AssignPendingToSlot(int slot);
    void DismissPendingSensor();

private:
    SettingsManager &settingsManager;
    InitState initState;
    RecursiveMutex mutex;
    Task task;

    void Work();
    void ScanBus();
    bool TriggerTemperatureConversions();
    bool ReadTemperatures();

    void LoadSlotAddresses();
    int FindSlotByAddress(uint64_t address);
    void ClearSlotHandles();

    static const char* GetSlotKey(int slot);
    static uint64_t ParseHexAddress(const char* str);
    static void FormatHexAddress(uint64_t addr, char* buf, size_t size);

    gpio_num_t one_wire_gpio = GPIO_NUM_4;
    onewire_bus_handle_t bus = nullptr;
    SensorSlot slots[MAX_SENSORS]{};

    uint64_t pendingAddresses[MAX_SENSORS]{};
    int pendingCount = 0;
};
