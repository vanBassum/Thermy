#pragma once
#include <esp_log.h>
#include <cstring>
#include "settings/settings.h"

class SettingsPrinter
{
    constexpr static const char *TAG = "SettingsPrinter";

public:
    void PrintGroup(ISettingsGroup &group, int indent = 0)
    {
        group.IterateSettings([&](SettingHandle h)
                              { PrintItem(h, indent); });
    }

    void PrintItem(SettingHandle &h, int indent = 0)
    {
        // Build indentation string
        char pad[64];
        int padLen = (indent < (int)sizeof(pad) - 1) ? indent : (int)sizeof(pad) - 1;
        memset(pad, ' ', padLen);
        pad[padLen] = '\0';

        switch (h.Type())
        {
        case SettingType::Unsigned8:
            ESP_LOGI(TAG, "%s%s = %u", pad, h.Key(), h.AsUInt8());
            break;
        case SettingType::Unsigned16:
            ESP_LOGI(TAG, "%s%s = %u", pad, h.Key(), h.AsUInt16());
            break;
        case SettingType::Unsigned32:
            ESP_LOGI(TAG, "%s%s = %lu", pad, h.Key(), h.AsUInt32());
            break;
        case SettingType::Integer8:
            ESP_LOGI(TAG, "%s%s = %d", pad, h.Key(), h.AsInt8());
            break;
        case SettingType::Integer16:
            ESP_LOGI(TAG, "%s%s = %d", pad, h.Key(), h.AsInt16());
            break;
        case SettingType::Integer32:
            ESP_LOGI(TAG, "%s%s = %ld", pad, h.Key(), h.AsInt32());
            break;
        case SettingType::Boolean:
            ESP_LOGI(TAG, "%s%s = %s", pad, h.Key(), h.AsBool() ? "true" : "false");
            break;
        case SettingType::Float:
            ESP_LOGI(TAG, "%s%s = %f", pad, h.Key(), h.AsFloat());
            break;
        case SettingType::String:
            ESP_LOGI(TAG, "%s%s = '%s'", pad, h.Key(), h.AsString());
            break;
        case SettingType::Enum:
            ESP_LOGI(TAG, "%s%s = %ld", pad, h.Key(), h.AsInt32());
            break;
        case SettingType::Blob:
            PrintBlob(TAG, pad, h.Key(), h.AsBlob(), h.Size());
            break;
        case SettingType::SettingGroup:
            ESP_LOGI(TAG, "%s%s:", pad, h.Key());
            PrintGroup(h.AsGroup(), indent + 2);
            break;
        default:
            ESP_LOGW(TAG, "%s%s = <unsupported>", pad, h.Key());
            break;
        }
    }

    static void PrintBlob(const char *tag, const char *pad, const char *key, const void *data, size_t size)
    {
        if (!data || size == 0)
        {
            ESP_LOGI(tag, "%s%s = [0 bytes] ()", pad, key);
            return;
        }

        // Limit printed size to prevent log overflow
        const size_t maxBytes = 64; 
        char hex[2 * maxBytes + 1];
        size_t printLen = size < maxBytes ? size : maxBytes;

        size_t pos = 0;
        const uint8_t *bytes = reinterpret_cast<const uint8_t *>(data);
        for (size_t i = 0; i < printLen && pos + 2 < sizeof(hex); ++i)
        {
            pos += snprintf(hex + pos, sizeof(hex) - pos, "%02X", bytes[i]);
        }
        hex[pos] = '\0';

        if (size > maxBytes)
        {
            ESP_LOGI(tag, "%s%s = [%u bytes] (%s...)", pad, key, (unsigned)size, hex);
        }
        else
        {
            ESP_LOGI(tag, "%s%s = [%u bytes] (%s)", pad, key, (unsigned)size, hex);
        }
    }
};
