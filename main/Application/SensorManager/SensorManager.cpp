#include "SensorManager.h"
#include "SettingsManager/SettingsManager.h"
#include "esp_check.h"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <cinttypes>
#include "core_utils.h"

SensorManager::SensorManager(ServiceProvider &ctx)
    : settingsManager(ctx.getSettingsManager())
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

    LoadSlotAddresses();

    task.Init("SensorManager", 6, 4096);
    task.SetHandler([this](){ Work(); });
    task.Run();

    init.SetReady();
}

// ── Slot access (thread-safe) ────────────────────────────────

float SensorManager::GetTemperature(int slot)
{
    WAIT_FOR_READY(initState);
    LOCK(mutex);
    if (slot < 0 || slot >= (int)MAX_SENSORS || !slots[slot].active)
        return 0.0f;
    return slots[slot].temperatureC;
}

uint64_t SensorManager::GetSlotAddress(int slot)
{
    WAIT_FOR_READY(initState);
    LOCK(mutex);
    if (slot < 0 || slot >= (int)MAX_SENSORS)
        return 0;
    return slots[slot].configuredAddress;
}

bool SensorManager::IsSlotActive(int slot)
{
    WAIT_FOR_READY(initState);
    LOCK(mutex);
    if (slot < 0 || slot >= (int)MAX_SENSORS)
        return false;
    return slots[slot].active;
}

// ── Pending sensor management ────────────────────────────────

bool SensorManager::HasPendingSensor()
{
    LOCK(mutex);
    return pendingCount > 0;
}

uint64_t SensorManager::GetPendingSensorAddress()
{
    LOCK(mutex);
    if (pendingCount == 0)
        return 0;
    return pendingAddresses[0];
}

void SensorManager::AssignPendingToSlot(int slot)
{
    LOCK(mutex);
    if (pendingCount == 0 || slot < 0 || slot >= (int)MAX_SENSORS)
        return;

    uint64_t address = pendingAddresses[0];

    // Update settings
    char hexBuf[20];
    FormatHexAddress(address, hexBuf, sizeof(hexBuf));
    settingsManager.setString(GetSlotKey(slot), hexBuf);
    settingsManager.Save();

    // Update slot and trigger rescan
    slots[slot].configuredAddress = address;
    rescanRequested = true;

    ESP_LOGI(TAG, "Assigned sensor %016" PRIX64 " to slot %d", address, slot);

    // Remove from pending queue (shift remaining)
    DismissPendingSensor();
}

void SensorManager::DismissPendingSensor()
{
    LOCK(mutex);
    if (pendingCount == 0)
        return;

    for (int i = 0; i < pendingCount - 1; i++)
        pendingAddresses[i] = pendingAddresses[i + 1];
    pendingCount--;
}

// ── Work loop ────────────────────────────────────────────────

void SensorManager::Work()
{
    TickType_t lastBusScan = 0;
    TickType_t lastTemperatureRead = 0;

    ScanBus();
    TriggerTemperatureConversions();

    while (1)
    {
        TickType_t now = xTaskGetTickCount();
        bool success = true;

        if (IsElapsed(now, lastTemperatureRead, TEMPERATURE_READ_INTERVAL))
        {
            success &= ReadTemperatures();
            success &= TriggerTemperatureConversions();
            lastTemperatureRead = now;
        }

        if (IsElapsed(now, lastBusScan, BUS_SCAN_INTERVAL) || (!success) || rescanRequested)
        {
            rescanRequested = false;
            ScanBus();
            TriggerTemperatureConversions();
            lastBusScan = now;
        }

        TickType_t busScanSleep = GetSleepTime(now, lastBusScan, BUS_SCAN_INTERVAL);
        TickType_t tempReadSleep = GetSleepTime(now, lastTemperatureRead, TEMPERATURE_READ_INTERVAL);
        TickType_t sleepTime = std::min(busScanSleep, tempReadSleep);
        vTaskDelay(sleepTime);
    }
}

// ── Bus scan ─────────────────────────────────────────────────

void SensorManager::ScanBus()
{
    // Enumerate into local buffers (no lock needed for bus I/O)
    struct DiscoveredSensor {
        ds18b20_device_handle_t handle;
        uint64_t address;
    };
    DiscoveredSensor discovered[MAX_SENSORS]{};
    int discoveredCount = 0;

    onewire_device_iter_handle_t iter = nullptr;
    ESP_ERROR_CHECK(onewire_new_device_iter(bus, &iter));

    onewire_device_t device;
    while (onewire_device_iter_get_next(iter, &device) == ESP_OK && discoveredCount < (int)MAX_SENSORS)
    {
        ds18b20_config_t ds_cfg = {};
        ds18b20_device_handle_t handle = nullptr;
        if (ds18b20_new_device_from_enumeration(&device, &ds_cfg, &handle) == ESP_OK)
        {
            onewire_device_address_t addr = 0;
            ds18b20_get_device_address(handle, &addr);
            discovered[discoveredCount].handle = handle;
            discovered[discoveredCount].address = addr;
            discoveredCount++;
        }
        else
        {
            ESP_LOGW(TAG, "Found non-DS18B20 device: %016" PRIX64, device.address);
        }
    }
    onewire_del_device_iter(iter);

    // Now take the lock briefly to reconcile with slots
    LOCK(mutex);

    // Reload addresses in case they changed (assignment from UI)
    LoadSlotAddresses();

    // Free old handles
    ClearSlotHandles();

    // Match discovered sensors to configured slots
    bool matched[MAX_SENSORS] = {};
    for (int d = 0; d < discoveredCount; d++)
    {
        int slot = FindSlotByAddress(discovered[d].address);
        if (slot >= 0)
        {
            slots[slot].handle = discovered[d].handle;
            slots[slot].active = true;
            matched[d] = true;
        }
    }

    // Unmatched discovered sensors go to pending queue
    pendingCount = 0;
    for (int d = 0; d < discoveredCount; d++)
    {
        if (!matched[d])
        {
            // Check it's not already pending or assigned
            if (pendingCount < (int)MAX_SENSORS)
            {
                pendingAddresses[pendingCount++] = discovered[d].address;
            }
            // Free handle for unassigned sensors
            ds18b20_del_device(discovered[d].handle);
        }
    }

    int activeCount = 0;
    for (int s = 0; s < (int)MAX_SENSORS; s++)
        if (slots[s].active) activeCount++;

    ESP_LOGI(TAG, "Scan: %d discovered, %d active slots, %d pending", discoveredCount, activeCount, pendingCount);
}

bool SensorManager::TriggerTemperatureConversions()
{
    LOCK(mutex);

    bool anyActive = false;
    for (int s = 0; s < (int)MAX_SENSORS; s++)
        if (slots[s].active) { anyActive = true; break; }

    if (!anyActive)
        return true;

    esp_err_t err = onewire_bus_reset(bus);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "OneWire reset failed: %s", esp_err_to_name(err));
        return false;
    }

    uint8_t cmd;
    cmd = 0xCC; // Skip ROM
    err = onewire_bus_write_bytes(bus, &cmd, 1);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to send Skip ROM: %s", esp_err_to_name(err));
        return false;
    }

    cmd = 0x44; // Convert T
    err = onewire_bus_write_bytes(bus, &cmd, 1);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to send Convert T: %s", esp_err_to_name(err));
        return false;
    }

    return true;
}

bool SensorManager::ReadTemperatures()
{
    LOCK(mutex);
    bool success = true;

    for (int s = 0; s < (int)MAX_SENSORS; s++)
    {
        if (!slots[s].active || !slots[s].handle)
            continue;

        esp_err_t err = ds18b20_get_temperature(slots[s].handle, &slots[s].temperatureC);
        if (err != ESP_OK)
        {
            success = false;
            slots[s].active = false;
            ESP_LOGE(TAG, "Failed to read slot %d: %s", s, esp_err_to_name(err));
        }
    }
    return success;
}

// ── Helpers ──────────────────────────────────────────────────

void SensorManager::LoadSlotAddresses()
{
    for (int s = 0; s < (int)MAX_SENSORS; s++)
    {
        char hexBuf[20] = {};
        if (settingsManager.getString(GetSlotKey(s), hexBuf, sizeof(hexBuf)) && hexBuf[0] != '\0')
            slots[s].configuredAddress = ParseHexAddress(hexBuf);
        else
            slots[s].configuredAddress = 0;
    }
}

int SensorManager::FindSlotByAddress(uint64_t address)
{
    if (address == 0) return -1;
    for (int s = 0; s < (int)MAX_SENSORS; s++)
    {
        if (slots[s].configuredAddress == address)
            return s;
    }
    return -1;
}

void SensorManager::ClearSlotHandles()
{
    for (int s = 0; s < (int)MAX_SENSORS; s++)
    {
        if (slots[s].handle)
        {
            ds18b20_del_device(slots[s].handle);
            slots[s].handle = nullptr;
        }
        slots[s].active = false;
    }
}

const char* SensorManager::GetSlotKey(int slot)
{
    static const char* keys[] = {"sensor.0", "sensor.1", "sensor.2", "sensor.3"};
    return keys[slot];
}

uint64_t SensorManager::ParseHexAddress(const char* str)
{
    return strtoull(str, nullptr, 16);
}

void SensorManager::FormatHexAddress(uint64_t addr, char* buf, size_t size)
{
    snprintf(buf, size, "%016" PRIX64, addr);
}
