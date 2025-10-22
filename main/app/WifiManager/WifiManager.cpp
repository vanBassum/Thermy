#include "WifiManager.h"
#include "SettingsManager.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include "core_utils.h"

WifiManager::WifiManager(ServiceProvider &ctx)
    : _ctx(ctx)
{
}

void WifiManager::Init()
{
    if (initGuard.IsReady())
        return;

    LOCK(mutex);

    if (!LoadSettings())
    {
        ESP_LOGW(TAG, "Wi-Fi disabled in settings, skipping initialization.");
        return;
    }

    ESP_LOGI(TAG, "Initializing ESP-IDF Wi-Fi...");
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &WifiManager::WifiEventHandler, this));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &WifiManager::WifiEventHandler, this));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    initGuard.SetReady();

    // Automatically connect after initialization
    Connect(pdMS_TO_TICKS(8000));
}

bool WifiManager::LoadSettings()
{
    _ctx.GetSettingsManager().Access([&](RootSettings &settings)
    {
        wifiEnabled = settings.system.wifiEnabled;
        memset(&wifiConfig, 0, sizeof(wifiConfig));
        memcpy(&wifiConfig.sta.ssid, settings.system.wifiSsid, MIN(sizeof(wifiConfig.sta.ssid), sizeof(settings.system.wifiSsid)));
        memcpy(&wifiConfig.sta.password, settings.system.wifiPassword, MIN(sizeof(wifiConfig.sta.password), sizeof(settings.system.wifiPassword)));
        wifiConfig.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
        wifiConfig.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;  // future-proof for mixed networks
    });

    if (!wifiEnabled)
        return false;

    ESP_LOGI(TAG, "Wi-Fi enabled: SSID=%s", wifiConfig.sta.ssid);
    return true;
}

bool WifiManager::Connect(TickType_t timeoutTicks)
{
    LOCK(mutex);

    if (!wifiEnabled)
    {
        ESP_LOGW(TAG, "Wi-Fi disabled, skipping connection.");
        return false;
    }

    if (wifiConfig.sta.ssid[0] == '\0')
    {
        ESP_LOGW(TAG, "No SSID configured.");
        return false;
    }

    ESP_LOGI(TAG, "Connecting to SSID: %s", wifiConfig.sta.ssid);
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifiConfig));
    ESP_ERROR_CHECK(esp_wifi_connect());

    uint32_t timeoutMs = (timeoutTicks == portMAX_DELAY)
                             ? 10000
                             : (timeoutTicks * portTICK_PERIOD_MS);

    connected = WaitForConnection(timeoutMs);

    if (connected)
        ESP_LOGI(TAG, "Wi-Fi connected successfully.");
    else
        ESP_LOGW(TAG, "Wi-Fi connection timeout.");

    return connected;
}

void WifiManager::Disconnect()
{
    LOCK(mutex);
    if (connected)
    {
        esp_wifi_disconnect();
        connected = false;
        ESP_LOGI(TAG, "Wi-Fi disconnected.");
    }
}

bool WifiManager::IsConnected() const
{
    return connected;
}

bool WifiManager::WaitForConnection(uint32_t timeoutMs)
{
    uint32_t start = esp_log_timestamp();
    while (!connected && (esp_log_timestamp() - start) < timeoutMs)
    {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    return connected;
}

void WifiManager::WifiEventHandler(void *arg, esp_event_base_t event_base,
                                   int32_t event_id, void *event_data)
{
    auto *self = static_cast<WifiManager *>(arg);

    if (event_base == WIFI_EVENT)
    {
        switch (event_id)
        {
        case WIFI_EVENT_STA_START:
            ESP_LOGI(TAG, "STA start event.");
            break;
        case WIFI_EVENT_STA_CONNECTED:
            ESP_LOGI(TAG, "Connected to AP.");
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
            ESP_LOGW(TAG, "Disconnected from AP.");
            self->connected = false;
            esp_wifi_connect();  // auto-reconnect
            break;
        default:
            break;
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        auto *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        self->connected = true;
    }
}

void WifiManager::Loop()
{
    if (!wifiEnabled)
        return;

    if (connected)
        return;

    uint32_t now = esp_log_timestamp();
    if (now - lastRetryMs < 30000) // 30s between retries
        return;

    lastRetryMs = now;
    ESP_LOGW(TAG, "Wi-Fi not connected, scheduling reconnect...");
    esp_wifi_connect();  // non-blocking reconnect
}

