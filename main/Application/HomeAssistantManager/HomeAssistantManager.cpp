#include "HomeAssistantManager.h"
#include "MqttManager/MqttManager.h"
#include "DeviceManager/DeviceManager.h"
#include "SensorManager/SensorManager.h"
#include "JsonWriter.h"
#include "BufferStream.h"
#include "esp_log.h"
#include <cstring>
#include <cstdio>

HomeAssistantManager::HomeAssistantManager(ServiceProvider &ctx)
    : serviceProvider_(ctx)
{
}

void HomeAssistantManager::Init()
{
    auto init = initState_.TryBeginInit();
    if (!init)
    {
        ESP_LOGW(TAG, "Already initialized or initializing");
        return;
    }

    auto &mqtt = serviceProvider_.getMqttManager();

    // ── LED light entity ─────────────────────────────────────

    mqtt.RegisterCommand("led", [this](const char *data, int len)
    {
        bool on = (len >= 2 && strncmp(data, "ON", 2) == 0);
        serviceProvider_.getDeviceManager().getLed().Set(on);
        PublishLedState();
    });

    mqtt.RegisterDiscovery([this]()
    {
        auto &mqtt = serviceProvider_.getMqttManager();

        mqtt.PublishEntityDiscovery("light", "led", [&mqtt](JsonWriter &json)
        {
            json.field("name", "LED");

            char topic[128];
            snprintf(topic, sizeof(topic), "%s/set/led", mqtt.GetBaseTopic());
            json.field("cmd_t", topic);

            snprintf(topic, sizeof(topic), "%s/led/state", mqtt.GetBaseTopic());
            json.field("stat_t", topic);

            json.field("pl_on", "ON");
            json.field("pl_off", "OFF");
        });

        PublishLedState();
    });

    // ── Temperature sensor entities ─────────────────────────

    mqtt.RegisterDiscovery([this]()
    {
        auto &mqtt = serviceProvider_.getMqttManager();

        char stateTopic[128];
        snprintf(stateTopic, sizeof(stateTopic), "%s/temperatures", mqtt.GetBaseTopic());

        for (int i = 0; i < MAX_SENSORS; i++)
        {
            char objectId[16];
            snprintf(objectId, sizeof(objectId), "temp_%d", i);

            char name[32];
            snprintf(name, sizeof(name), "Temperature %s", SLOT_NAMES[i]);

            char valTpl[48];
            snprintf(valTpl, sizeof(valTpl), "{{ value_json.t%d }}", i);

            mqtt.PublishEntityDiscovery("sensor", objectId, [&](JsonWriter &json)
            {
                json.field("name", name);

                json.field("stat_t", stateTopic);
                json.field("val_tpl", valTpl);
                json.field("dev_cla", "temperature");
                json.field("unit_of_meas", "\u00b0C");
                json.field("sug_dsp_prc", static_cast<int32_t>(1));
            });
        }

        PublishTemperatures();
    });

    publishTimer_.Init("ha_temp", pdMS_TO_TICKS(30000), true);
    publishTimer_.SetHandler([this]()
    {
        PublishTemperatures();
    });
    publishTimer_.Start();

    init.SetReady();
    ESP_LOGI(TAG, "Initialized");
}

void HomeAssistantManager::PublishLedState()
{
    bool on = serviceProvider_.getDeviceManager().getLed().IsOn();
    serviceProvider_.getMqttManager().Publish("led/state", on ? "ON" : "OFF", true);
}

void HomeAssistantManager::PublishTemperatures()
{
    auto &mqtt = serviceProvider_.getMqttManager();
    if (!mqtt.IsConnected())
        return;

    auto &sensors = serviceProvider_.getSensorManager();

    char buf[128];
    BufferStream stream(buf, sizeof(buf));
    JsonWriter json(stream);
    json.beginObject();
    for (int i = 0; i < MAX_SENSORS; i++)
    {
        char key[4];
        snprintf(key, sizeof(key), "t%d", i);
        if (sensors.IsSlotActive(i))
            json.field(key, sensors.GetTemperature(i));
        else
            json.nullField(key);
    }
    json.endObject();

    mqtt.Publish("temperatures", buf);
}
