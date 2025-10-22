#pragma once
#include "settings.h"
#include "secrets.h"

// ======================================================
// Network settings
// ======================================================

struct SystemSettings : public ISettingsGroup
{
    using ThisType = SystemSettings;

    // WiFi settings
    bool wifiEnabled;
    char wifiSsid[32];
    char wifiPassword[64];

    char influxBaseUrl[128];
    char influxApiKey[128];
    char influxOrganisation[64];
    char influxBucket[64];    

    uint32_t oneWireGpio;

    static const SettingsDescriptor SCHEMA[];

    const SettingsDescriptor *GetSettingsSchema() const override;
    int GetSettingsCount() const override;
};

// ------------------------------------------------------
// Static schema definition
// ------------------------------------------------------
inline const SettingsDescriptor SystemSettings::SCHEMA[] = {
    DESCRIPTOR_FIELD("wifi_ena",    wifiEnabled,        true),
    DESCRIPTOR_FIELD("wifi_ssid",   wifiSsid,           WIFI_SSID),
    DESCRIPTOR_FIELD("wifi_pwd",    wifiPassword,       WIFI_PASSWORD),
    DESCRIPTOR_FIELD("influx_url",  influxBaseUrl,      INFLUX_BASE_URL),
    DESCRIPTOR_FIELD("influx_key",  influxApiKey,       INFLUX_API_KEY),
    DESCRIPTOR_FIELD("influx_org",  influxOrganisation, INFLUX_ORGANISATION),
    DESCRIPTOR_FIELD("influx_bkt",  influxBucket,       INFLUX_BUCKET),
    DESCRIPTOR_FIELD("onewire_gpio", oneWireGpio,       2),

};


// ------------------------------------------------------
// Function implementations
// ------------------------------------------------------
inline const SettingsDescriptor *SystemSettings::GetSettingsSchema() const
{
    return SCHEMA;
}

inline int SystemSettings::GetSettingsCount() const
{
    return static_cast<int>(sizeof(SCHEMA) / sizeof(SCHEMA[0]));
}