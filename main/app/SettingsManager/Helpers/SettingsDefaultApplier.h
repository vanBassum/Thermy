#pragma once
#include <esp_log.h>
#include <cstring>
#include "settings/settings.h"

class SettingsDefaultApplier {
    constexpr static const char *TAG = "SettingsDefaultApplier";

public:
    static void ApplyGroup(ISettingsGroup &group) {
        group.IterateSettings([](SettingHandle h) {
            ApplyOne(h);
        });
    }

    static void ApplyOne(SettingHandle &handle) {
        switch (handle.Type()) {
        case SettingType::Unsigned8:
        case SettingType::Unsigned16:
        case SettingType::Unsigned32:
        case SettingType::Integer8:
        case SettingType::Integer16:
        case SettingType::Integer32:
        case SettingType::Enum:
        case SettingType::Boolean:
        case SettingType::Float:
            std::memcpy(handle.Ptr(), &handle.Default(), handle.Size());
            break;

        case SettingType::String: {
            const char *def = handle.Default().str ? handle.Default().str : "";
            size_t len = std::strlen(def);
            size_t copyLen = (len < handle.Size() - 1) ? len : (handle.Size() - 1);
            char *dst = handle.AsString();
            std::memcpy(dst, def, copyLen);
            std::memset(dst + copyLen, 0, handle.Size() - copyLen);
            break;
        }

        case SettingType::Blob: {
            if (handle.Default().blob) {
                const uint8_t *src = reinterpret_cast<const uint8_t *>(handle.Default().blob);
                std::memcpy(handle.Ptr(), src, handle.Size());
            } else {
                std::memset(handle.Ptr(), 0, handle.Size());
            }
            break;
        }

        case SettingType::SettingGroup:
            ApplyGroup(handle.AsGroup());
            break;

        default:
            ESP_LOGE(TAG, "Unsupported type %d in ApplyOne for key %s", static_cast<int>(handle.Type()), handle.Key());
            assert(false && "Unsupported type");
            break;
        }
    }
};
