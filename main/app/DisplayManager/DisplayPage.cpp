#include "DisplayPage.h"
#include "SettingsManager.h"
#include "esp_system.h"
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstring>

static constexpr const char *TAG = "DisplayPage";

void DisplayPage::SaveAndReboot(SettingsManager &settings)
{
    settings.Save();
    ESP_LOGI(TAG, "Settings saved, rebooting...");
    vTaskDelay(pdMS_TO_TICKS(500));
    esp_restart();
}

// ── Settings access via the registry chain ───────────────────

Setting *DisplayPage::FindSetting(SettingsManager &settings, const char *key)
{
    for (Setting &s : settings)
    {
        if (strcmp(s.key, key) == 0)
            return &s;
    }
    ESP_LOGE(TAG, "setting '%s' is not registered", key);
    return nullptr;
}

void DisplayPage::ReadSettingText(SettingsManager &settings, const char *key,
                                  char *out, size_t cap)
{
    if (cap == 0)
        return;
    out[0] = '\0';

    Setting *s = FindSetting(settings, key);
    if (!s)
        return;

    switch (s->type)
    {
    case SettingType::String:
        s->asString().Get(out, cap);
        break;
    case SettingType::Int32:
        snprintf(out, cap, "%" PRId32, s->asInt32().Get());
        break;
    case SettingType::UInt32:
        snprintf(out, cap, "%" PRIu32, s->asUInt32().Get());
        break;
    case SettingType::Float:
        snprintf(out, cap, "%.2f", static_cast<double>(s->asFloat().Get()));
        break;
    case SettingType::Bool:
        snprintf(out, cap, "%s", s->asBool().Get() ? "1" : "0");
        break;
    }
}

void DisplayPage::WriteSettingText(SettingsManager &settings, const char *key,
                                   const char *text)
{
    Setting *s = FindSetting(settings, key);
    if (!s)
        return;

    switch (s->type)
    {
    case SettingType::String:
        s->asString().Set(text);
        break;
    case SettingType::Int32:
        s->asInt32().Set(static_cast<int32_t>(strtol(text, nullptr, 10)));
        break;
    case SettingType::UInt32:
        s->asUInt32().Set(static_cast<uint32_t>(strtoul(text, nullptr, 10)));
        break;
    case SettingType::Float:
        s->asFloat().Set(strtof(text, nullptr));
        break;
    case SettingType::Bool:
        // Accept both the "1"/"0" this page renders and a typed-in word.
        s->asBool().Set(text[0] == '1' || text[0] == 't' || text[0] == 'T');
        break;
    }
}

int32_t DisplayPage::ReadSettingInt(SettingsManager &settings, const char *key, int32_t def)
{
    Setting *s = FindSetting(settings, key);
    if (!s)
        return def;

    switch (s->type)
    {
    case SettingType::Int32:  return s->asInt32().Get();
    case SettingType::UInt32: return static_cast<int32_t>(s->asUInt32().Get());
    case SettingType::Float:  return static_cast<int32_t>(s->asFloat().Get());
    case SettingType::Bool:   return s->asBool().Get() ? 1 : 0;
    case SettingType::String: break;
    }

    ESP_LOGE(TAG, "setting '%s' is not numeric", key);
    return def;
}
