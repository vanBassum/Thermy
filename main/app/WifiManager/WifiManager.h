#pragma once
#include "ServiceProvider.h"
#include "InitGuard.h"
#include "RecursiveMutex.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "TickContext.h"

class WifiManager
{
    inline static constexpr const char *TAG = "WifiManager";

public:
    explicit WifiManager(ServiceProvider &ctx);

    void Init();
    void Tick(TickContext& ctx) {}
    bool Connect(TickType_t timeoutTicks = portMAX_DELAY);
    void Disconnect();
    bool IsConnected() const;
    bool GetIp(char *outBuffer, size_t bufferLen) const;

private:
    ServiceProvider &_ctx;
    InitGuard initGuard;
    RecursiveMutex mutex;
    bool connected = false;

    // Cached configuration
    bool wifiEnabled = false;
    wifi_config_t wifiConfig{};

    bool LoadSettings();
    bool WaitForConnection(uint32_t timeoutMs);
    void init_mdns();

    // Internal event handler
    static void WifiEventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
};
