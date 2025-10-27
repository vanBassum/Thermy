#pragma once
#include "settings.h"
#include "secrets.h"

// ======================================================
// Network settings
// ======================================================

struct RuntimeVariables : public ISettingsGroup
{
    using ThisType = RuntimeVariables;

    // WiFi settings
    char uiSha256[65];

    static const SettingsDescriptor SCHEMA[];

    const SettingsDescriptor *GetSettingsSchema() const override;
    int GetSettingsCount() const override;
};

// ------------------------------------------------------
// Static schema definition
// ------------------------------------------------------
inline const SettingsDescriptor RuntimeVariables::SCHEMA[] = {
    DESCRIPTOR_FIELD("ui_sha256", uiSha256, ""),
};


// ------------------------------------------------------
// Function implementations
// ------------------------------------------------------
inline const SettingsDescriptor *RuntimeVariables::GetSettingsSchema() const
{
    return SCHEMA;
}

inline int RuntimeVariables::GetSettingsCount() const
{
    return static_cast<int>(sizeof(SCHEMA) / sizeof(SCHEMA[0]));
}