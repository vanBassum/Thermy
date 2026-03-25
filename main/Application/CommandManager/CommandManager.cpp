#include "CommandManager.h"
#include "LogManager.h"
#include "SettingsManager.h"
#include "UpdateManager.h"
#include "JsonWriter.h"
#include "JsonHelpers.h"
#include "esp_log.h"
#include "esp_app_desc.h"
#include "esp_system.h"
#include "esp_heap_caps.h"
#include "NetworkManager.h"
#include "SensorManager.h"
#include "TemperatureHistory.h"
#include <cstring>

const CommandManager::CommandEntry CommandManager::commands_[] = {
    { "ping",         &CommandManager::Cmd_Ping },
    { "info",         &CommandManager::Cmd_Info },
    { "updateStatus",  &CommandManager::Cmd_UpdateStatus },
    { "getSettings",   &CommandManager::Cmd_GetSettings },
    { "setSetting",    &CommandManager::Cmd_SetSetting },
    { "saveSettings",  &CommandManager::Cmd_SaveSettings },
    { "reboot",        &CommandManager::Cmd_Reboot },
    { "wifiScan",      &CommandManager::Cmd_WifiScan },
    { "getLogs",       &CommandManager::Cmd_GetLogs },
    { "getTemperatures", &CommandManager::Cmd_GetTemperatures },
    { "getHistory",      &CommandManager::Cmd_GetHistory },
    { nullptr, nullptr },
};

CommandManager::CommandManager(ServiceProvider& serviceProvider)
    : serviceProvider_(serviceProvider)
{
}

void CommandManager::Init()
{
    auto initAttempt = initState_.TryBeginInit();
    if (!initAttempt)
    {
        ESP_LOGW(TAG, "Already initialized or initializing");
        return;
    }

    initAttempt.SetReady();
    ESP_LOGI(TAG, "Initialized");
}

bool CommandManager::Execute(const char* type, const char* json, JsonWriter& resp)
{
    for (int i = 0; commands_[i].type != nullptr; i++)
    {
        if (strcmp(type, commands_[i].type) == 0)
        {
            (this->*commands_[i].func)(json, resp);
            return true;
        }
    }

    return false;
}

// ──────────────────────────────────────────────────────────────
// Commands
// ──────────────────────────────────────────────────────────────

void CommandManager::Cmd_Ping(const char* json, JsonWriter& resp)
{
    resp.field("pong", true);
}

void CommandManager::Cmd_Info(const char* json, JsonWriter& resp)
{
    const esp_app_desc_t* app = esp_app_get_description();

    resp.field("project", app->project_name);
    resp.field("firmware", app->version);
    resp.field("idf", app->idf_ver);
    resp.field("date", app->date);
    resp.field("time", app->time);
    resp.field("chip", CONFIG_IDF_TARGET);
    resp.field("heapFree", static_cast<uint32_t>(esp_get_free_heap_size()));
    resp.field("heapMin", static_cast<uint32_t>(esp_get_minimum_free_heap_size()));
}

void CommandManager::Cmd_UpdateStatus(const char* json, JsonWriter& resp)
{
    const esp_app_desc_t* app = esp_app_get_description();
    auto& update = serviceProvider_.getUpdateManager();

    resp.field("firmware", app->version);
    resp.field("running", update.GetRunningPartition());
    resp.field("nextSlot", update.GetNextPartition());
}

void CommandManager::Cmd_GetSettings(const char* json, JsonWriter& resp)
{
    serviceProvider_.getSettingsManager().WriteAllSettings(resp);
}

void CommandManager::Cmd_SetSetting(const char* json, JsonWriter& resp)
{
    char key[64] = {};
    char value[128] = {};
    ExtractJsonString(json, "key", key, sizeof(key));
    ExtractJsonString(json, "value", value, sizeof(value));

    if (key[0] == '\0')
    {
        resp.field("ok", false);
        resp.field("error", "missing key");
        return;
    }

    auto& settings = serviceProvider_.getSettingsManager();
    const auto* defs = settings.GetDefinitions();
    int count = settings.GetDefinitionCount();

    for (int i = 0; i < count; i++)
    {
        if (strcmp(defs[i].key, key) == 0)
        {
            switch (defs[i].type)
            {
            case SettingType::String:
                settings.setString(key, value);
                break;
            case SettingType::Int:
                settings.setInt(key, atoi(value));
                break;
            case SettingType::Bool:
                settings.setBool(key, strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
                break;
            }

            resp.field("ok", true);
            return;
        }
    }

    resp.field("ok", false);
    resp.field("error", "unknown key");
}

void CommandManager::Cmd_SaveSettings(const char* json, JsonWriter& resp)
{
    bool ok = serviceProvider_.getSettingsManager().Save();
    resp.field("ok", ok);
}

void CommandManager::Cmd_Reboot(const char* json, JsonWriter& resp)
{
    resp.field("ok", true);
    // Delay to allow WS response to be sent before restarting
    vTaskDelay(pdMS_TO_TICKS(500));
    esp_restart();
}

void CommandManager::Cmd_WifiScan(const char* json, JsonWriter& resp)
{
    auto& wifi = serviceProvider_.getNetworkManager().wifi();

    WiFiInterface::ScanResult results[20] = {};
    int count = wifi.Scan(results, 20);

    resp.field("ok", true);
    resp.fieldArray("networks");

    for (int i = 0; i < count; i++)
    {
        resp.beginObject();
        resp.field("ssid", results[i].ssid);
        resp.field("rssi", static_cast<int32_t>(results[i].rssi));
        resp.field("channel", static_cast<int32_t>(results[i].channel));
        resp.field("secure", results[i].secure);
        resp.endObject();
    }

    resp.endArray();
}

void CommandManager::Cmd_GetLogs(const char* json, JsonWriter& resp)
{
    serviceProvider_.getLogManager().WriteHistory(resp);
}

void CommandManager::Cmd_GetTemperatures(const char* json, JsonWriter& resp)
{
    auto& sensors = serviceProvider_.getSensorManager();

    resp.fieldArray("sensors");
    for (int i = 0; i < 4; i++)
    {
        resp.beginObject();
        resp.field("slot", static_cast<int32_t>(i));
        resp.field("active", sensors.IsSlotActive(i));

        char addrBuf[20] = {};
        uint64_t addr = sensors.GetSlotAddress(i);
        if (addr != 0)
            snprintf(addrBuf, sizeof(addrBuf), "%016llX", addr);
        resp.field("address", addrBuf);

        resp.field("temperature", sensors.IsSlotActive(i) ? sensors.GetTemperature(i) : 0.0f);
        resp.endObject();
    }
    resp.endArray();
}

void CommandManager::Cmd_GetHistory(const char* json, JsonWriter& resp)
{
    auto& history = serviceProvider_.getTemperatureHistory();

    int32_t count = 360;
    ExtractJsonInt(json, "count", count);
    if (count < 1) count = 1;
    if (count > (int32_t)TemperatureHistory::MAX_SAMPLES) count = TemperatureHistory::MAX_SAMPLES;

    // Allocate on stack — limit to 360 to keep stack usage reasonable
    if (count > 360) count = 360;
    TemperatureSample samples[360];
    size_t n = history.GetSamples(samples, count);

    resp.field("maxSamples", static_cast<int32_t>(TemperatureHistory::MAX_SAMPLES));
    resp.field("rate", serviceProvider_.getSettingsManager().getInt("history.rate", TemperatureHistory::DEFAULT_RATE_SECONDS));
    resp.field("count", static_cast<int32_t>(n));
    resp.fieldArray("samples");
    for (size_t i = 0; i < n; i++)
    {
        resp.beginObject();
        for (int s = 0; s < 4; s++)
        {
            char key[4];
            snprintf(key, sizeof(key), "s%d", s);
            if (samples[i].IsActive(s))
                resp.field(key, samples[i].temperatures[s]);
            else
                resp.nullField(key);
        }
        resp.endObject();
    }
    resp.endArray();
}
