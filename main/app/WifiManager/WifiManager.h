#pragma once
#include "ServiceProvider.h"
#include "InitGuard.h"
#include "RecursiveMutex.h"
#include "esp_wifi.h"
#include "esp_event.h"

class WifiManager
{
    inline static constexpr const char *TAG = "WifiManager";

public:
    explicit WifiManager(ServiceProvider &ctx);

    void Init();
    bool Connect(TickType_t timeoutTicks = portMAX_DELAY);
    void Disconnect();
    bool IsConnected() const;
    void Loop();

private:
    ServiceProvider &_ctx;
    InitGuard initGuard;
    RecursiveMutex mutex;
    bool connected = false;

    // Cached configuration
    bool wifiEnabled = false;
    wifi_config_t wifiConfig{};
    uint32_t lastRetryMs = 0;

    bool LoadSettings();
    bool WaitForConnection(uint32_t timeoutMs);

    // Internal event handler
    static void WifiEventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
};
