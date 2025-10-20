#pragma once
#include "settings.h"
#include "NetworkSettings.h"

// ======================================================
// Root settings
// ======================================================

struct RootSettings : public ISettingsGroup
{
    using ThisType = RootSettings;

    NetworkSettings network;

    static const SettingsDescriptor SCHEMA[];

    const SettingsDescriptor *GetSettingsSchema() const override;
    int GetSettingsCount() const override;
};

// ------------------------------------------------------
// Static schema definition
// ------------------------------------------------------
inline const SettingsDescriptor RootSettings::SCHEMA[] = {
    DESCRIPTOR_GROUP("Network", network),
};

// ------------------------------------------------------
// Function implementations
// ------------------------------------------------------
inline const SettingsDescriptor *RootSettings::GetSettingsSchema() const
{
    return SCHEMA;
}

inline int RootSettings::GetSettingsCount() const
{
    return static_cast<int>(sizeof(SCHEMA) / sizeof(SCHEMA[0]));
}