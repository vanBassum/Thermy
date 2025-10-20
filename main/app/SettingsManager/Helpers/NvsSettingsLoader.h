#pragma once
#include <esp_log.h>
#include <cstring>
#include <cassert>
#include "NvsStorage.h"
#include "settings.h"
#include "SettingsDefaultApplier.h"

class NvsSettingsLoader
{
    constexpr static const char *TAG = "NvsSettingsLoader";
    const char* partitionName;

public:
    explicit NvsSettingsLoader(const char* partition)
        : partitionName(partition)
    {}

    // The root is expected to contain only groups, not fields.
    esp_err_t Load(ISettingsGroup &root)
    {
        esp_err_t result = ESP_OK;

        root.IterateSettings([&](SettingHandle h)
        {
            if (h.Type() != SettingType::SettingGroup)
            {
                ESP_LOGE(TAG, "Root-level field '%s' not supported", h.Key());
                assert(false && "Root-level field not supported for NVS storage");
                result = ESP_ERR_INVALID_ARG;
                return;
            }

            LoadGroup(h.AsGroup(), h.Key());
        });

        return result;
    }

    // Allow loading a single group (namespace)
    void LoadGroup(ISettingsGroup &group, const char *groupName)
    {
        NvsStorage storage(partitionName, groupName);
        storage.Open();

        group.IterateSettings([&](SettingHandle h)
        {
            if (h.Type() == SettingType::SettingGroup)
            {
                ESP_LOGE(TAG, "Nested group '%s' not supported in NVS (namespace/key only)", h.Key());
                assert(false && "Nested groups not supported in NVS storage");
                return;
            }

            if (TryLoad(storage, h) != ESP_OK)
            {
                SettingsDefaultApplier::ApplyOne(h);
            }
        });

        storage.Close();
    }

private:
    esp_err_t TryLoad(NvsStorage &storage, SettingHandle &h)
    {
        switch (h.Type())
        {
        case SettingType::Unsigned8:  return storage.GetUInt8(h.Key(), h.AsUInt8());
        case SettingType::Unsigned16: return storage.GetUInt16(h.Key(), h.AsUInt16());
        case SettingType::Unsigned32: return storage.GetUInt32(h.Key(), h.AsUInt32());
        case SettingType::Integer8:   return storage.GetInt8(h.Key(), h.AsInt8());
        case SettingType::Integer16:  return storage.GetInt16(h.Key(), h.AsInt16());
        case SettingType::Integer32:  return storage.GetInt32(h.Key(), h.AsInt32());
        case SettingType::Enum:       return storage.GetInt32(h.Key(), h.AsInt32());
        case SettingType::Boolean:    return storage.GetBool(h.Key(), h.AsBool());
        case SettingType::Float:      return storage.GetFloat(h.Key(), h.AsFloat());

        case SettingType::String:
        {
            size_t len = 0;
            esp_err_t err = storage.GetString(h.Key(), nullptr, 0, len);
            if (err != ESP_OK)
                return err;

            if (len >= h.Size())
            {
                ESP_LOGE(TAG, "String too large for key '%s' (len=%u, max=%u)",
                         h.Key(), (unsigned)len, (unsigned)h.Size());
                return ESP_ERR_INVALID_SIZE;
            }

            err = storage.GetString(h.Key(), h.AsString(), h.Size(), len);
            if (err == ESP_OK)
            {
                char *dst = h.AsString();
                if (len < h.Size())
                    std::memset(dst + len, 0, h.Size() - len);
            }
            return err;
        }

        case SettingType::Blob:
        {
            size_t len = 0;
            esp_err_t err = storage.GetBlob(h.Key(), nullptr, 0, len);
            if (err != ESP_OK)
                return err;

            if (len > h.Size())
            {
                ESP_LOGE(TAG, "Blob too large for key '%s' (len=%u, max=%u)",
                         h.Key(), (unsigned)len, (unsigned)h.Size());
                return ESP_ERR_INVALID_SIZE;
            }

            err = storage.GetBlob(h.Key(), h.AsBlob(), h.Size(), len);
            if (err == ESP_OK && len < h.Size())
            {
                uint8_t *dst = reinterpret_cast<uint8_t *>(h.AsBlob());
                std::memset(dst + len, 0, h.Size() - len);
            }
            return err;
        }

        default:
            ESP_LOGE(TAG, "Unsupported type %d for key '%s'",
                     static_cast<int>(h.Type()), h.Key());
            assert(false && "Unsupported setting type");
            return ESP_ERR_INVALID_ARG;
        }
    }
};
