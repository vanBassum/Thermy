#include "BoardContext.h"
#include "esp_log.h"

void BoardContext::Init()
{
    auto init = initState_.TryBeginInit();
    if (!init)
    {
        ESP_LOGW(TAG, "Already initialized or initializing");
        return;
    }

    // The bus host comes up before the drivers that use it.
    InitOneWire();

    display_.Init();

    init.SetReady();
    ESP_LOGI(TAG, "Initialized");
}

void BoardContext::InitOneWire()
{
    onewire_bus_config_t busCfg = {
        .bus_gpio_num = BoardConfig::ONEWIRE_PIN,
        .flags = { .en_pull_up = 1 },
    };
    onewire_bus_rmt_config_t rmtCfg = {
        .max_rx_bytes = 10,   // 1-Wire ROM search needs 10 bytes
    };

    esp_err_t err = onewire_new_bus_rmt(&busCfg, &rmtCfg, &oneWireBus_);
    if (err != ESP_OK)
    {
        // Not fatal: the web UI, telemetry and the display are all still
        // worth having on a device whose probe bus is unplugged.
        ESP_LOGW(TAG, "1-Wire bus init failed on GPIO%d: %s — no probes",
                 (int)BoardConfig::ONEWIRE_PIN, esp_err_to_name(err));
        oneWireBus_ = nullptr;
        return;
    }

    ESP_LOGI(TAG, "1-Wire bus ready on GPIO%d", (int)BoardConfig::ONEWIRE_PIN);
}
