#pragma once
#include "settings.h"

// ======================================================
// Network settings
// ======================================================

struct NetworkSettings : public ISettingsGroup
{
    using ThisType = NetworkSettings;

    // WiFi settings
    bool wifiEnabled;
    char wifiSsid[32];
    char wifiPassword[64];

    

    static const SettingsDescriptor SCHEMA[];

    const SettingsDescriptor *GetSettingsSchema() const override;
    int GetSettingsCount() const override;
};

// ------------------------------------------------------
// Static schema definition
// ------------------------------------------------------
inline const SettingsDescriptor NetworkSettings::SCHEMA[] = {
    DESCRIPTOR_FIELD("wifi_ena", wifiEnabled, true),
    DESCRIPTOR_FIELD("wifi_ssid", wifiSsid, ""),
    DESCRIPTOR_FIELD("wifi_pwd", wifiPassword, ""),
};


// ------------------------------------------------------
// Function implementations
// ------------------------------------------------------
inline const SettingsDescriptor *NetworkSettings::GetSettingsSchema() const
{
    return SCHEMA;
}

inline int NetworkSettings::GetSettingsCount() const
{
    return static_cast<int>(sizeof(SCHEMA) / sizeof(SCHEMA[0]));
}