#pragma once
#include "settings.h"
#include <cstring>
#include <esp_log.h>

class SettingsPathResolver
{
    constexpr static const char *TAG = "SettingsPathResolver";

public:
    bool Resolve(const ISettingsGroup& startGroup, const char* path, SettingHandle& out)
    {
        const ISettingsGroup* current = &startGroup;
        const char* ptr = path;

        while (true)
        {
            SettingHandle h;
            if (!FindInGroup(*current, ptr, h))
                return false;

            const char* slash = strchr(ptr, '/');
            if (!slash)
            {
                // Last segment → this is our result
                out = h;
                return true;
            }

            // There are more segments; must go deeper
            if (h.Type() != SettingType::SettingGroup)
            {
                ESP_LOGE(TAG, "Path segment '%.*s' is not a group", (int)(slash - ptr), ptr);
                return false;
            }

            current = &h.AsGroup();
            ptr = slash + 1; // move past slash and continue
        }
    }



private:

    bool FindInGroup(const ISettingsGroup& group, const char* key, SettingHandle& out)
    {
        // Find / or \0
        const char* slash = strchr(key, '/');
        const size_t len = slash ? (size_t)(slash - key) : strlen(key);
        bool found = false;

        group.IterateSettings([&](SettingHandle h)
        {
            const char* name = h.Key();
            if (std::memcmp(name, key, len) == 0 && name[len] == '\0')
            {
                out = h;
                found = true;
                return;
            }
        });

        if (!found)
            ESP_LOGW(TAG, "Key '%.*s' not found", static_cast<int>(len), key);

        return found;
    }
};

