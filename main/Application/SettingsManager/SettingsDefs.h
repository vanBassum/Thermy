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
    { "device.name",   SettingType::String, "Device Name",   "Thermy" },
    { "device.pin",    SettingType::String, "Device PIN",    "" },

    // Time
    { "ntp.server",    SettingType::String, "NTP Server",    "pool.ntp.org" },
    { "ntp.timezone",  SettingType::String, "Timezone (POSIX)", "CET-1CEST,M3.5.0,M10.5.0/3" },

    // Temperature history & graph
    { "history.rate",  SettingType::Int, "History Sample Rate (s)", "10" },
    { "graph.min",     SettingType::Int, "Graph Min Temperature",   "0" },
    { "graph.max",     SettingType::Int, "Graph Max Temperature",   "100" },

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
