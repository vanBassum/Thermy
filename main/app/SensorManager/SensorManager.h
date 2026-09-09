#pragma once

#include "AppProvider.h"
#include "InitState.h"
#include "CommandEntry.h"
#include "UiModule.h"
#include "TypedSettings.h"
#include "rtos.h"
#include "onewire_bus.h"
#include "ds18b20.h"
#include <cstdint>

class Stream;

// ──────────────────────────────────────────────────────────────
// DS18B20 temperature probes on the board's 1-Wire bus, held in a fixed
// set of named slots.
//
// Slots exist because a 1-Wire address is not a position: the probe in the
// tank and the probe on the flow pipe are interchangeable to the bus and
// very much not to the user. A slot binds one ROM address to one meaning,
// persisted in NVS, so relabelling survives a reboot and a rescan.
//
// A probe found on the bus that no slot claims becomes *pending* — offered
// to the UI for assignment rather than silently adopted. That is what makes
// "plug a probe in and tell it where it is" the whole setup flow.
//
// The bus host itself belongs to the board (BoardContext::GetOneWireBus), so this
// manager names no GPIO.
// ──────────────────────────────────────────────────────────────

struct SensorSlot
{
    uint64_t configuredAddress = 0;   // from settings (0 = unassigned)
    ds18b20_device_handle_t handle = nullptr;
    float temperatureC = 0.0f;
    bool active = false;              // true if the probe was found on the bus
};

class SensorManager
{
    static constexpr const char* TAG = "SensorManager";

public:
    static constexpr size_t MAX_SENSORS = 4;

    explicit SensorManager(AppProvider& app);

    SensorManager(const SensorManager&) = delete;
    SensorManager& operator=(const SensorManager&) = delete;
    SensorManager(SensorManager&&) = delete;
    SensorManager& operator=(SensorManager&&) = delete;

    void Init();

    // ── Slot access (thread-safe; called from the LVGL task too) ──
    float GetTemperature(int slot);
    uint64_t GetSlotAddress(int slot);
    bool IsSlotActive(int slot);

    // ── Pending probe management ──────────────────────────────
    bool HasPendingSensor();
    uint64_t GetPendingSensorAddress();
    void AssignPendingToSlot(int slot);
    void DismissPendingSensor();
    void ClearAllSlots();

private:
    AppProvider& app_;
    InitState initState_;
    RecursiveMutex mutex_;
    Task task_;

    onewire_bus_handle_t bus_ = nullptr;   // lent by the board, never owned
    SensorSlot slots_[MAX_SENSORS]{};

    uint64_t pendingAddresses_[MAX_SENSORS]{};
    int pendingCount_ = 0;
    bool rescanRequested_ = false;

    void Work();
    void ScanBus();
    bool TriggerTemperatureConversions();
    bool ReadTemperatures();

    /// One telemetry point per active slot. Lives here rather than in a
    /// separate monitor: this manager already wakes on the read interval,
    /// and a point is cheap enough that a second task to take it would be
    /// pure overhead.
    void PublishTelemetry();

    void LoadSlotAddresses();
    int FindSlotByAddress(uint64_t address);
    void ClearSlotHandles();

    static StringSetting& SlotSetting(int slot);
    static uint64_t ParseHexAddress(const char* str);
    static void FormatHexAddress(uint64_t addr, char* buf, size_t size);

    // ── Settings (registered with SettingsManager in Init) ──
    // NVS keys are capped at 15 characters — Register() asserts at runtime.
    inline static StringSetting slot0_{ "sensor.0", "Sensor Slot 1 Address", "" };
    inline static StringSetting slot1_{ "sensor.1", "Sensor Slot 2 Address", "" };
    inline static StringSetting slot2_{ "sensor.2", "Sensor Slot 3 Address", "" };
    inline static StringSetting slot3_{ "sensor.3", "Sensor Slot 4 Address", "" };

    inline static UInt32Setting scanIntervalMs_{ "sensor.scan", "Bus Scan Interval (ms)", 5000 };
    inline static UInt32Setting readIntervalMs_{ "sensor.read", "Read Interval (ms)", 1000 };
    // Label distinguishes this from TelemetryManager's own `telem.interval`, which
    // paces the device vitals point — both would otherwise read "Telemetry
    // Interval (s)" in the generated settings UI.
    inline static UInt32Setting telemetrySec_{ "sensor.telem", "Probe Telemetry Interval (s)", 60 };

    // ── Commands (registered with CommandManager in Init) ──
    RequestError Cmd_List(CommandContext& ctx);
    RequestError Cmd_Assign(CommandContext& ctx);
    RequestError Cmd_Clear(CommandContext& ctx);

    inline static CommandEntry commands_[] = {
        { "sensor", "list",   &InvokeCommand<&SensorManager::Cmd_List> },
        { "sensor", "assign", &InvokeCommand<&SensorManager::Cmd_Assign> },
        { "sensor", "clear",  &InvokeCommand<&SensorManager::Cmd_Clear> },
    };

    // ── The web UI (registered with UiManager in Init) ──
    // The probes ARE the product, so this is the page the shell lands on. Not
    // because anything here says so: UiManager head-inserts and the application
    // layer initialises after the framework, so Thermy's module ends up ahead of
    // Strux's own console/settings/firmware and the first declared page wins.
    // There is no separate home page to land on instead — a device with one
    // feature has nothing to put on one.
    //
    // `entry` is a path inside the device's own www, named by the firmware and
    // matched by frontend/modules/temperature/. The page id must equal the one the
    // bundle registers in its activate().
    inline static const UiPage uiPages_[] = { { "temperature", "Temperature", "thermometer" } };
    inline static UiModule uiModule_{ "temperature", "/modules/temperature.js", uiPages_ };
};
