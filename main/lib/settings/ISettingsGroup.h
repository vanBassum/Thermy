#pragma once
#include <cstddef>
#include "SettingsDescriptor.h"
#include "SettingHandle.h"


struct ISettingsGroup
{
    virtual const SettingsDescriptor *GetSettingsSchema() const = 0;
    virtual int GetSettingsCount() const = 0;

    template <typename FUNC>
    void IterateSettings(FUNC &&callback)
    {
        SettingsDescriptor const *schema = GetSettingsSchema();
        int count = GetSettingsCount();
        for (int i = 0; i < count; i++)
        {
            SettingHandle handle{schema[i], *this};
            callback(handle);
        }
    }

    template <typename FUNC>
    void IterateSettings(FUNC &&callback) const
    {
        SettingsDescriptor const *schema = GetSettingsSchema();
        int count = GetSettingsCount();
        for (int i = 0; i < count; i++)
        {
            const SettingHandle handle{schema[i], const_cast<ISettingsGroup &>(*this)};
            callback(handle);
        }
    }
};

