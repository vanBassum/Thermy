#pragma once
#include "NvsStorage.h"
#include "settings.h"
#include <esp_log.h>


class NvsSettingsStorer
{
    constexpr static const char *TAG = "NvsSettingsStorer";
    const char* partitionName;

public:
    explicit NvsSettingsStorer(const char* partition)
        : partitionName(partition)
    {}

    esp_err_t Store(const ISettingsGroup &root)
    {
        esp_err_t result = ESP_OK;

        // The root is expected to have only groups, no direct fields
        root.IterateSettings([&](SettingHandle h)
        {
            if (h.Type() != SettingType::SettingGroup)
            {
                ESP_LOGE(TAG, "Root-level field '%s' not supported", h.Key());
                assert(false && "Root-level field not supported for NVS storage");
                result = ESP_ERR_INVALID_ARG;
                return; // just exit this iteration
            }

            // Each subgroup becomes its own NVS namespace
            StoreGroup(h.AsGroup(), h.Key());
        });

        return result;
    }


    void StoreGroup(const ISettingsGroup& group, const char* groupName)
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

            esp_err_t err = TryStore(storage, h);
            if (err != ESP_OK)
                ESP_LOGE(TAG, "Failed to store key '%s' in group '%s': 0x%x", h.Key(), groupName, err);
        });

        storage.Commit();
        storage.Close();
    }

private:

    esp_err_t TryStore(NvsStorage& storage, const SettingHandle &h)
    {
        switch (h.Type())
        {
        case SettingType::Unsigned8:  return storage.SetUInt8(h.Key(), h.AsUInt8());
        case SettingType::Unsigned16: return storage.SetUInt16(h.Key(), h.AsUInt16());
        case SettingType::Unsigned32: return storage.SetUInt32(h.Key(), h.AsUInt32());
        case SettingType::Integer8:   return storage.SetInt8(h.Key(), h.AsInt8());
        case SettingType::Integer16:  return storage.SetInt16(h.Key(), h.AsInt16());
        case SettingType::Integer32:  return storage.SetInt32(h.Key(), h.AsInt32());
        case SettingType::Enum:       return storage.SetInt32(h.Key(), h.AsInt32());
        case SettingType::Boolean:    return storage.SetBool(h.Key(), h.AsBool());
        case SettingType::Float:      return storage.SetFloat(h.Key(), h.AsFloat());
        case SettingType::String:     return storage.SetString(h.Key(), h.AsString());
        case SettingType::Blob:       return storage.SetBlob(h.Key(), h.AsBlob(), h.Size());
        default:
            ESP_LOGE(TAG, "Unsupported type %d for key '%s'", static_cast<int>(h.Type()), h.Key());
            assert(false && "Unsupported setting type");
            return ESP_ERR_INVALID_ARG;
        }
    }
};
