#include "DisplayManager.h"
#include "WifiManager.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "font8x8sym.h"

DisplayManager::DisplayManager(ServiceProvider &ctx)
    : sensorManager(ctx.GetSensorManager())
    , wifiManager(ctx.GetWifiManager())
    , timeManager(ctx.GetTimeManager())
{
}

void DisplayManager::Init()
{
    if (initGuard.IsReady())
        return;

    LOCK(mutex);

    driver.Init();
    initGuard.SetReady();
    ESP_LOGI(TAG, "DisplayManager initialized.");
}

void DisplayManager::Tick(TickContext &ctx)
{
    REQUIRE_READY(initGuard);
    LOCK(mutex);

    if (!ctx.ElapsedAndReset(lastDisplayUpdate, DISPLAY_UPDATE_INTERVAL))
        return;

    SSD1306 &display = driver.GetDisplay();
    display.fill(0);

    DrawIcons(display);
    DrawSensorTemperatures(display);

    display.show();
}

void DisplayManager::DrawIcons(SSD1306 &display)
{
    TextStyle iconStyle(&font8x8sym, 1, true);

    // Left column of icons
    const int iconX = 72-10;
    int y = 0;

    // --- WiFi icon (top)
    char wifiChar = wifiManager.IsConnected() ? (char)SymbolIcon::Wifi : (char)SymbolIcon::Empty;
    display.drawChar(iconX, y, wifiChar, iconStyle);

    // --- NTP synced icon
    char ntpChar = timeManager.HasSynced() ? (char)SymbolIcon::Smyle : (char)SymbolIcon::Empty;
    display.drawChar(iconX, y+=10, ntpChar, iconStyle);

}

void DisplayManager::DrawSensorTemperatures(SSD1306 &display)
{
    const int sensorCount = sensorManager.GetSensorCount();
    if (sensorCount <= 0)
        return;

    const uint8_t textSize = sensorCount <= 2 ? 2 : 1;
    const int lineHeight = sensorCount <= 2 ? 18 : 9;
    const int baseY = 0;
    const int textX = 0;

    TextStyle tempStyle(&Font5x7, textSize, true);

    for (int i = 0; i < sensorCount; ++i)
    {
        float tempC = sensorManager.GetTemperature(i);

        char line[16];
        if (tempC >= 100.0f)
            snprintf(line, sizeof(line), "%.1f", tempC);
        else
            snprintf(line, sizeof(line), "%.2f", tempC);

        int y = baseY + i * lineHeight;
        display.drawText(textX, y, line, tempStyle);
    }
}

