#pragma once
#include <cstring>
#include <esp_log.h>
#include "settings.h"
#include "StringConverter.h"

class SettingsStringGetter
{
    constexpr static const char *TAG = "SettingsStringGetter";
    StringConverter& converter;

public:
    explicit SettingsStringGetter(StringConverter& converter)
        : converter(converter)
    {
    }

    bool GetAsString(ISettingsGroup& group, const char* key, char* buffer, size_t size)
    {
        bool success = false;

        group.IterateSettings([&](SettingHandle h)
        {
            if (strcmp(h.Key(), key) == 0)
                success = RetrieveStringValue(h, buffer, size);
        });

        if (!success)
            ESP_LOGW(TAG, "Setting '%s' not found or invalid", key);

        return success;
    }

    bool RetrieveStringValue(SettingHandle& h, char* buffer, size_t size)
    {
        switch (h.Type())
        {
        case SettingType::Unsigned8:
            return converter.UInt8ToString(buffer, size, h.AsUInt8());
        case SettingType::Unsigned16:
            return converter.UInt16ToString(buffer, size, h.AsUInt16());
        case SettingType::Unsigned32:
            return converter.UInt32ToString(buffer, size, h.AsUInt32());
        case SettingType::Integer8:
            return converter.Int8ToString(buffer, size, h.AsInt8());
        case SettingType::Integer16:
            return converter.Int16ToString(buffer, size, h.AsInt16());
        case SettingType::Integer32:
            return converter.Int32ToString(buffer, size, h.AsInt32());
        case SettingType::Enum:
            return converter.EnumToString(buffer, size, h.AsInt32());
        case SettingType::Boolean:
            return converter.BoolToString(buffer, size, h.AsBool());
        case SettingType::Float:
            return converter.FloatToString(buffer, size, h.AsFloat());
        case SettingType::Blob:
            return converter.BlobToString(buffer, size, h.AsBlob(), h.Size());
        case SettingType::String:
        {
            std::snprintf(buffer, size, "%s", h.AsString());
            return true;
        }

        default:
            ESP_LOGE(TAG, "Unsupported type %d for key '%s'", static_cast<int>(h.Type()), h.Key());
            return false;
        }
    }
};
