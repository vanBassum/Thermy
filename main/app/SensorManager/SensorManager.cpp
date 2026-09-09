#include "SensorManager.h"
#include "SettingsManager.h"
#include "CommandManager.h"
#include "TelemetryManager.h"
#include "UiManager.h"
#include "BoardContext.h"
#include "Fatal.h"
#include "core_utils.h"
#include "esp_log.h"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <cinttypes>

SensorManager::SensorManager(AppProvider& app)
    : app_(app)
{
}

void SensorManager::Init()
{
    auto initAttempt = initState_.TryBeginInit();
    if (!initAttempt)
    {
        ESP_LOGW(TAG, "Already initialized or initializing");
        return;
    }

    app_.getStrux().getSettingsManager().Register({
        &slot0_, &slot1_, &slot2_, &slot3_,
        &scanIntervalMs_, &readIntervalMs_, &telemetrySec_,
    });
    app_.getStrux().getCommandManager().Register(this, commands_);
    app_.getStrux().getUiManager().Register({ &uiModule_ });

    // Borrowed, not owned — the board created the bus host and outlives us.
    bus_ = app_.getBoard().GetOneWireBus();
    if (!bus_)
    {
        // Still mark ready: the slots read as inactive, the UI shows no probes,
        // and everything else on the device works.
        ESP_LOGW(TAG, "No 1-Wire bus — sensors disabled");
        initAttempt.SetReady();
        return;
    }

    LoadSlotAddresses();

    task_.Init("SensorManager", 6, 4096);
    task_.SetHandler([this]() { Work(); });
    task_.Run();

    initAttempt.SetReady();
    ESP_LOGI(TAG, "Initialized");
}

// ── Slot access (thread-safe) ────────────────────────────────

float SensorManager::GetTemperature(int slot)
{
    WAIT_FOR_READY(initState_);
    LOCK(mutex_);
    if (slot < 0 || slot >= (int)MAX_SENSORS || !slots_[slot].active)
        return 0.0f;
    return slots_[slot].temperatureC;
}

uint64_t SensorManager::GetSlotAddress(int slot)
{
    WAIT_FOR_READY(initState_);
    LOCK(mutex_);
    if (slot < 0 || slot >= (int)MAX_SENSORS)
        return 0;
    return slots_[slot].configuredAddress;
}

bool SensorManager::IsSlotActive(int slot)
{
    WAIT_FOR_READY(initState_);
    LOCK(mutex_);
    if (slot < 0 || slot >= (int)MAX_SENSORS)
        return false;
    return slots_[slot].active;
}

// ── Pending probe management ─────────────────────────────────

bool SensorManager::HasPendingSensor()
{
    LOCK(mutex_);
    return pendingCount_ > 0;
}

uint64_t SensorManager::GetPendingSensorAddress()
{
    LOCK(mutex_);
    if (pendingCount_ == 0)
        return 0;
    return pendingAddresses_[0];
}

void SensorManager::AssignPendingToSlot(int slot)
{
    LOCK(mutex_);
    if (pendingCount_ == 0 || slot < 0 || slot >= (int)MAX_SENSORS)
        return;

    uint64_t address = pendingAddresses_[0];

    char hexBuf[20];
    FormatHexAddress(address, hexBuf, sizeof(hexBuf));
    SlotSetting(slot).Set(hexBuf);
    app_.getStrux().getSettingsManager().Save();

    slots_[slot].configuredAddress = address;
    rescanRequested_ = true;

    ESP_LOGI(TAG, "Assigned sensor %016" PRIX64 " to slot %d", address, slot);

    DismissPendingSensor();
}

void SensorManager::DismissPendingSensor()
{
    LOCK(mutex_);
    if (pendingCount_ == 0)
        return;

    for (int i = 0; i < pendingCount_ - 1; i++)
        pendingAddresses_[i] = pendingAddresses_[i + 1];
    pendingCount_--;
}

void SensorManager::ClearAllSlots()
{
    LOCK(mutex_);
    for (int s = 0; s < (int)MAX_SENSORS; s++)
    {
        SlotSetting(s).Set("");
        slots_[s].configuredAddress = 0;
        if (slots_[s].handle)
        {
            ds18b20_del_device(slots_[s].handle);
            slots_[s].handle = nullptr;
        }
        slots_[s].active = false;
        slots_[s].temperatureC = 0.0f;
    }
    pendingCount_ = 0;
    app_.getStrux().getSettingsManager().Save();
    rescanRequested_ = true;
    ESP_LOGI(TAG, "All sensor slots cleared");
}

// ── Work loop ────────────────────────────────────────────────

void SensorManager::Work()
{
    TickType_t lastBusScan = 0;
    TickType_t lastTemperatureRead = 0;
    TickType_t lastTelemetry = xTaskGetTickCount();

    ScanBus();
    TriggerTemperatureConversions();

    while (1)
    {
        TickType_t scanInterval = pdMS_TO_TICKS(scanIntervalMs_.Get());
        TickType_t readInterval = pdMS_TO_TICKS(readIntervalMs_.Get());
        TickType_t telemetryInterval = pdMS_TO_TICKS(telemetrySec_.Get() * 1000);

        TickType_t now = xTaskGetTickCount();
        bool success = true;

        if (IsElapsed(now, lastTemperatureRead, readInterval))
        {
            success &= ReadTemperatures();
            success &= TriggerTemperatureConversions();
            lastTemperatureRead = now;
        }

        if (IsElapsed(now, lastBusScan, scanInterval) || (!success) || rescanRequested_)
        {
            rescanRequested_ = false;
            ScanBus();
            TriggerTemperatureConversions();
            lastBusScan = now;
        }

        if (IsElapsed(now, lastTelemetry, telemetryInterval))
        {
            PublishTelemetry();
            lastTelemetry = now;
        }

        TickType_t busScanSleep = GetSleepTime(now, lastBusScan, scanInterval);
        TickType_t tempReadSleep = GetSleepTime(now, lastTemperatureRead, readInterval);
        TickType_t telemetrySleep = GetSleepTime(now, lastTelemetry, telemetryInterval);
        TickType_t sleepTime = std::min({ busScanSleep, tempReadSleep, telemetrySleep });
        vTaskDelay(sleepTime);
    }
}

// ── Bus scan ─────────────────────────────────────────────────

void SensorManager::ScanBus()
{
    // Enumerate into local buffers (no lock needed for bus I/O)
    struct DiscoveredSensor
    {
        ds18b20_device_handle_t handle;
        uint64_t address;
    };
    DiscoveredSensor discovered[MAX_SENSORS]{};
    int discoveredCount = 0;

    onewire_device_iter_handle_t iter = nullptr;
    if (onewire_new_device_iter(bus_, &iter) != ESP_OK)
    {
        ESP_LOGW(TAG, "Could not start 1-Wire enumeration");
        return;
    }

    onewire_device_t device;
    while (onewire_device_iter_get_next(iter, &device) == ESP_OK &&
           discoveredCount < (int)MAX_SENSORS)
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
    LOCK(mutex_);

    // Reload addresses in case they changed (assignment from the UI)
    LoadSlotAddresses();

    // Free old handles
    ClearSlotHandles();

    // Match discovered probes to configured slots
    bool matched[MAX_SENSORS] = {};
    for (int d = 0; d < discoveredCount; d++)
    {
        int slot = FindSlotByAddress(discovered[d].address);
        if (slot >= 0)
        {
            slots_[slot].handle = discovered[d].handle;
            slots_[slot].active = true;
            matched[d] = true;
        }
    }

    // Unmatched probes go to the pending queue
    pendingCount_ = 0;
    for (int d = 0; d < discoveredCount; d++)
    {
        if (!matched[d])
        {
            if (pendingCount_ < (int)MAX_SENSORS)
                pendingAddresses_[pendingCount_++] = discovered[d].address;

            // Free the handle for unassigned probes
            ds18b20_del_device(discovered[d].handle);
        }
    }

    if (pendingCount_ > 0)
        ESP_LOGI(TAG, "Scan: %d new sensor(s) found", pendingCount_);
}

bool SensorManager::TriggerTemperatureConversions()
{
    LOCK(mutex_);

    bool anyActive = false;
    for (int s = 0; s < (int)MAX_SENSORS; s++)
        if (slots_[s].active) { anyActive = true; break; }

    if (!anyActive)
        return true;

    esp_err_t err = onewire_bus_reset(bus_);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "OneWire reset failed: %s", esp_err_to_name(err));
        return false;
    }

    uint8_t cmd;
    cmd = 0xCC; // Skip ROM
    err = onewire_bus_write_bytes(bus_, &cmd, 1);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to send Skip ROM: %s", esp_err_to_name(err));
        return false;
    }

    cmd = 0x44; // Convert T
    err = onewire_bus_write_bytes(bus_, &cmd, 1);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to send Convert T: %s", esp_err_to_name(err));
        return false;
    }

    return true;
}

bool SensorManager::ReadTemperatures()
{
    LOCK(mutex_);
    bool success = true;

    for (int s = 0; s < (int)MAX_SENSORS; s++)
    {
        if (!slots_[s].active || !slots_[s].handle)
            continue;

        esp_err_t err = ds18b20_get_temperature(slots_[s].handle, &slots_[s].temperatureC);
        if (err != ESP_OK)
        {
            success = false;
            slots_[s].active = false;
            ESP_LOGE(TAG, "Failed to read slot %d: %s", s, esp_err_to_name(err));
        }
    }
    return success;
}

void SensorManager::PublishTelemetry()
{
    auto& telemetry = app_.getStrux().getTelemetryManager();

    for (int s = 0; s < (int)MAX_SENSORS; s++)
    {
        float value;
        {
            LOCK(mutex_);
            if (!slots_[s].active)
                continue;
            value = slots_[s].temperatureC;
        }

        // One point per slot, tagged by slot, so a dashboard can graph the
        // probes apart without knowing their ROM addresses.
        char slotTag[8];
        snprintf(slotTag, sizeof(slotTag), "%d", s);

        auto point = telemetry.Measure("temperature");
        point.Tag("slot", slotTag);
        point.Field("celsius", static_cast<double>(value));
        point.Commit();
    }
}

// ── Commands ─────────────────────────────────────────────────

RequestError SensorManager::Cmd_List(CommandContext& ctx)
{
    RETURN_IF_ERROR(ctx.readArgs());

    auto resp = ctx.reply.object();
    {
        auto slots = resp.array("slots");
        for (int s = 0; s < (int)MAX_SENSORS; s++)
        {
            uint64_t address;
            bool active;
            float value;
            {
                LOCK(mutex_);
                address = slots_[s].configuredAddress;
                active = slots_[s].active;
                value = slots_[s].temperatureC;
            }

            char hexBuf[20] = {};
            if (address != 0)
                FormatHexAddress(address, hexBuf, sizeof(hexBuf));

            auto slot = slots.object();
            slot.field("slot", static_cast<int32_t>(s));
            slot.field("address", hexBuf);
            slot.field("active", active);
            if (active)
                slot.field("celsius", value);
        }
    }

    uint64_t pending;
    {
        LOCK(mutex_);
        pending = pendingCount_ > 0 ? pendingAddresses_[0] : 0;
    }
    char pendingBuf[20] = {};
    if (pending != 0)
        FormatHexAddress(pending, pendingBuf, sizeof(pendingBuf));
    resp.field("pending", pendingBuf);

    return RequestError::Ok;
}

RequestError SensorManager::Cmd_Assign(CommandContext& ctx)
{
    uint32_t slot = 0;
    RETURN_IF_ERROR(ctx.readArgs(Required("slot", slot)));

    auto resp = ctx.reply.object();

    // Meaning, not form — an out-of-range slot goes in the reply, not a REJECT.
    if (slot >= MAX_SENSORS)
    {
        resp.field("ok", false);
        resp.field("error", "slot out of range");
        return RequestError::Ok;
    }

    if (!HasPendingSensor())
    {
        resp.field("ok", false);
        resp.field("error", "no pending sensor");
        return RequestError::Ok;
    }

    AssignPendingToSlot(static_cast<int>(slot));
    resp.field("ok", true);
    return RequestError::Ok;
}

RequestError SensorManager::Cmd_Clear(CommandContext& ctx)
{
    RETURN_IF_ERROR(ctx.readArgs());

    ClearAllSlots();

    auto resp = ctx.reply.object();
    resp.field("ok", true);
    return RequestError::Ok;
}

// ── Helpers ──────────────────────────────────────────────────

void SensorManager::LoadSlotAddresses()
{
    for (int s = 0; s < (int)MAX_SENSORS; s++)
    {
        char hexBuf[20] = {};
        SlotSetting(s).Get(hexBuf, sizeof(hexBuf));
        slots_[s].configuredAddress = hexBuf[0] != '\0' ? ParseHexAddress(hexBuf) : 0;
    }
}

int SensorManager::FindSlotByAddress(uint64_t address)
{
    if (address == 0) return -1;
    for (int s = 0; s < (int)MAX_SENSORS; s++)
    {
        if (slots_[s].configuredAddress == address)
            return s;
    }
    return -1;
}

void SensorManager::ClearSlotHandles()
{
    for (int s = 0; s < (int)MAX_SENSORS; s++)
    {
        if (slots_[s].handle)
        {
            ds18b20_del_device(slots_[s].handle);
            slots_[s].handle = nullptr;
        }
        slots_[s].active = false;
    }
}

StringSetting& SensorManager::SlotSetting(int slot)
{
    static StringSetting* const table[MAX_SENSORS] = { &slot0_, &slot1_, &slot2_, &slot3_ };
    if (slot < 0 || slot >= (int)MAX_SENSORS)
        FATAL("sensor slot %d out of range", slot);
    return *table[slot];
}

uint64_t SensorManager::ParseHexAddress(const char* str)
{
    return strtoull(str, nullptr, 16);
}

void SensorManager::FormatHexAddress(uint64_t addr, char* buf, size_t size)
{
    snprintf(buf, size, "%016" PRIX64, addr);
}
