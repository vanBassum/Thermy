#pragma once
#include <cstddef>
#include <cstdint>
#include "SettingType.h"


union DefaultValue
{
    const char *str;
    int32_t i;
    bool b;
    float f;
    const void *blob;
};

struct ISettingsGroup; // forward

struct SettingsDescriptor
{
    const char *key;
    SettingType type;
    size_t offset;
    size_t size;
    DefaultValue defaultVal;

    void *GetPtr(ISettingsGroup &obj) const
    {
        return reinterpret_cast<uint8_t *>(&obj) + offset;
    }
    const void *GetPtr(const ISettingsGroup &obj) const
    {
        return reinterpret_cast<const uint8_t *>(&obj) + offset;
    }
};
