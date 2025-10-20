#pragma once
#include <cstring>
#include <esp_log.h>
#include "settings.h"
#include "StringConverter.h"

class SettingsStringSetter
{
    constexpr static const char *TAG = "SettingsStringSetter";
    StringConverter& converter;

public:
    explicit SettingsStringSetter(StringConverter& converter)
        : converter(converter)
    {
    }

    bool SetFromString(ISettingsGroup& group, const char* key, const char* value)
    {
        bool success = false;

        group.IterateSettings([&](SettingHandle h)
        {
            if (strcmp(h.Key(), key) == 0)
                success = ApplyStringValue(h, value);
        });

        if (!success)
            ESP_LOGW(TAG, "Setting '%s' not found or invalid", key);

        return success;
    }

    bool ApplyStringValue(SettingHandle& handle, const char* value)
    {
        switch (handle.Type())
        {
        case SettingType::Unsigned8:
            return converter.StringToUInt8(handle.AsUInt8(), value);
        case SettingType::Unsigned16:
            return converter.StringToUInt16(handle.AsUInt16(), value);
        case SettingType::Unsigned32:
            return converter.StringToUInt32(handle.AsUInt32(), value);
        case SettingType::Integer8:
            return converter.StringToInt8(handle.AsInt8(), value);
        case SettingType::Integer16:
            return converter.StringToInt16(handle.AsInt16(), value);
        case SettingType::Integer32:
            return converter.StringToInt32(handle.AsInt32(), value);
        case SettingType::Enum:
            return converter.StringToEnum(handle.AsInt32(), value);
        case SettingType::Boolean:
            return converter.StringToBool(handle.AsBool(), value);
        case SettingType::Float:
            return converter.StringToFloat(handle.AsFloat(), value);
        case SettingType::Blob:
            return converter.StringToBlob(handle.AsBlob(), handle.Size(), value);
        case SettingType::String:
        {
            size_t len = strlen(value);
            if (len >= handle.Size())
            {
                ESP_LOGE(TAG, "String too large for key '%s' (len=%u, max=%u)", handle.Key(), static_cast<unsigned>(len), static_cast<unsigned>(handle.Size() - 1));
                return false;
            }

            std::memset(handle.AsString(), 0, handle.Size());
            std::memcpy(handle.AsString(), value, len);
            return true;
        }

        default:
            ESP_LOGE(TAG, "Unsupported type %d for key '%s'", static_cast<int>(handle.Type()), handle.Key());
            return false;
        }
    }
};
