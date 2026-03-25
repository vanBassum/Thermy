#pragma once

#include "SettingsManager.h"

// ──────────────────────────────────────────────────────────────
// Setting definitions — add new settings here
// ──────────────────────────────────────────────────────────────

inline const SettingDef SETTINGS_DEFS[] = {
    // WiFi
    { "wifi.ssid",     SettingType::String, "WiFi SSID",     "" },
    { "wifi.password", SettingType::String, "WiFi Password", "" },

    // Device
    { "device.name",   SettingType::String, "Device Name",   "Skeleton" },

    // Sensor timing (milliseconds)
    { "sensor.scan",   SettingType::Int, "Bus Scan Interval (ms)",  "5000" },
    { "sensor.read",   SettingType::Int, "Temp Read Interval (ms)", "1000" },

    // Sensor slot addresses (hex string of 64-bit OneWire address, empty = unassigned)
    { "sensor.0",      SettingType::String, "Sensor Slot 1 (Red)",    "" },
    { "sensor.1",      SettingType::String, "Sensor Slot 2 (Blue)",   "" },
    { "sensor.2",      SettingType::String, "Sensor Slot 3 (Green)",  "" },
    { "sensor.3",      SettingType::String, "Sensor Slot 4 (Yellow)", "" },
};

inline constexpr int SETTINGS_DEFS_COUNT = sizeof(SETTINGS_DEFS) / sizeof(SETTINGS_DEFS[0]);
