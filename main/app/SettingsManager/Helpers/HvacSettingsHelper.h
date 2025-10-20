#pragma once
#include "HvacSettings.h"

class HvacSettingsHelper
{
public:
    explicit HvacSettingsHelper(HvacSettings& hvac)
        : _hvac(hvac)
    {
        Bind();
    }

    char* const* GetArray() const { return _hvacArray; }
    size_t Count() const { return HvacSettings::HVAC_COUNT; }

    bool Add(const char* src)
    {
        for (auto& hvac : _hvacArray)
        {
            if (hvac[0] == '\0')
            {
                strncpy(hvac, src, HvacSettings::HVAC_STRING_SIZE - 1);
                hvac[HvacSettings::HVAC_STRING_SIZE - 1] = '\0';
                return true;
            }
        }
        return false;
    }

    void ClearAll()
    {
        for (auto& hvac : _hvacArray)
            hvac[0] = '\0';
    }

    const char* Get(int index) const
    {
        if (index < 0 || index >= HvacSettings::HVAC_COUNT)
            return "";
        return _hvacArray[index];
    }

private:
    HvacSettings& _hvac;
    char* _hvacArray[HvacSettings::HVAC_COUNT]{};

    void Bind()
    {
        int i = 0;
        _hvacArray[i++] = _hvac.hvac1;
        _hvacArray[i++] = _hvac.hvac2;
        _hvacArray[i++] = _hvac.hvac3;
        _hvacArray[i++] = _hvac.hvac4;
        _hvacArray[i++] = _hvac.hvac5;
        
        // This is here, so that if HVAC_COUNT is changed, we get a compile-time error to update this code as well.
        static_assert(HvacSettings::HVAC_COUNT == 5, "HvacSettingsHelper: HVAC_COUNT mismatch — update bindings.");
    }
};
