#include "DisplayManager.h"
#include "WifiManager.h"
#include "esp_log.h"
#include "esp_netif.h"

DisplayManager::DisplayManager(ServiceProvider &ctx)
    : _ctx(ctx)
{
}

void DisplayManager::Init()
{
    if (initGuard.IsReady())
        return;

    LOCK(mutex);

    driver.Init();
    ShowSplashScreen();

    lastUpdate = esp_log_timestamp();
    initGuard.SetReady();

    ESP_LOGI(TAG, "DisplayManager initialized.");
}

void DisplayManager::ShowSplashScreen()
{
    driver.Clear();
    driver.PrintLine(0, "=== Thermy ===");
    driver.PrintLine(2, "Starting...");
}

// ----------------------------------------------------------------------------
// Internal helper for printing IP address
// ----------------------------------------------------------------------------
static void PrintIpAddress(DisplayDriver &driver)
{
    esp_netif_ip_info_t ipInfo;
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (netif && esp_netif_get_ip_info(netif, &ipInfo) == ESP_OK)
    {
        char ipStr[32];
        snprintf(ipStr, sizeof(ipStr), IPSTR, IP2STR(&ipInfo.ip));
        driver.Clear();
        driver.PrintLine(0, "Wi-Fi connected");
        driver.PrintLine(1, ipStr);
        ESP_LOGI("DisplayManager", "Showing IP: %s", ipStr);
    }
    else
    {
        driver.Clear();
        driver.PrintLine(0, "Wi-Fi not ready");
    }
}

// ----------------------------------------------------------------------------
// Loop
// ----------------------------------------------------------------------------
void DisplayManager::Loop()
{
    if (!initGuard.IsReady())
        return;

    uint32_t now = esp_log_timestamp();
    uint32_t elapsed = now - lastUpdate;

    // Phase 1: Show IP for first 5 seconds after Wi-Fi connects
    static bool showedIp = false;
    static bool showingIp = false;
    static uint32_t ipStartTime = 0;

    auto &wifi = _ctx.GetWifiManager();

    if (!showedIp && wifi.IsConnected())
    {
        PrintIpAddress(driver);
        showingIp = true;
        showedIp = true;
        ipStartTime = now;
        return;
    }

    // Wait 5 seconds showing the IP before switching to temperature
    if (showingIp && (now - ipStartTime) < 5000)
        return;

    if (showingIp && (now - ipStartTime) >= 5000)
    {
        showingIp = false;
        driver.Clear();
    }

    // Phase 2: show temperature periodically
    if (elapsed < 1000)
        return;

    lastUpdate = now;

    LOCK(mutex);
    driver.PrintLine(0, "Thermy");
    driver.PrintLine(1, "Wi-Fi: OK");
    driver.PrintLine(2, "T1: 23.4C");
}

void DisplayManager::Clear()
{
    driver.Clear();
}

void DisplayManager::PrintLine(int line, const char *text)
{
    driver.PrintLine(line, text);
}
