#include "DisplayManager.h"
#include "WifiManager.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "font8x8sym.h"

DisplayManager::DisplayManager(ServiceProvider &ctx)
    : sensorManager(ctx.GetSensorManager())
    , wifiManager(ctx.GetWifiManager())
    , timeManager(ctx.GetTimeManager())
    , influxManager(ctx.GetInfluxManager())
    , hardwareManager(ctx.GetHardwareManager())
{
}

void DisplayManager::Init()
{
    if (initGuard.IsReady())
        return;

    LOCK(mutex);

    display.Init();//hardwareManager.GetI2CBus());
    initGuard.SetReady();
    ESP_LOGI(TAG, "DisplayManager initialized.");
}

void DisplayManager::Tick(TickContext &ctx)
{
    REQUIRE_READY(initGuard);
    LOCK(mutex);

    if (!ctx.HasElapsed(lastDisplayUpdate, DISPLAY_UPDATE_INTERVAL))
        return;

    display.fill(0);

    DrawIcons(display);
    DrawSensorTemperatures(display);

    DateTime now = DateTime::Now();
    char dateStr[12];
    char timeStr[10];
    now.ToStringLocal(dateStr, sizeof(dateStr), "%d-%m-%Y");
    now.ToStringLocal(timeStr, sizeof(timeStr), "%H:%M:%S");
    display.drawText(0, 250-30, dateStr, TextStyle(&Font5x7, 2, true));
    display.drawText(0, 250-14, timeStr, TextStyle(&Font5x7, 2, true));

    display.show();
    ctx.MarkExecuted(lastDisplayUpdate, DISPLAY_UPDATE_INTERVAL);
}

void DisplayManager::DrawIcons(Display &display)
{
    TextStyle iconStyle(&font8x8sym, 1, true);

    // Left column of icons
    const int iconX = 0;
    int y = 0;

    // --- WiFi icon (top)
    char wifiChar = wifiManager.IsConnected() ? (char)SymbolIcon::Wifi : (char)SymbolIcon::Empty;
    display.drawChar(iconX, y, wifiChar, iconStyle);

    // --- NTP synced icon
    char ntpChar = timeManager.HasSynced() ? (char)SymbolIcon::Smyle : (char)SymbolIcon::Empty;
    display.drawChar(iconX, y+=10, ntpChar, iconStyle);

    // --- Influx icon
    char influxChar = influxManager.IsWorking() ? (char)SymbolIcon::Influx : (char)SymbolIcon::Empty;
    display.drawChar(iconX, y+=10, influxChar, iconStyle);

    // --- Cycle tick icon
    rotator++;
    rotator %= 8;
    char tickChar = (char)SymbolIcon::Rot_1 + rotator;
    display.drawChar(iconX, y+=10, tickChar, iconStyle);

}

void DisplayManager::DrawSensorTemperatures(Display &display)
{
    const int sensorCount = sensorManager.GetSensorCount();
    if (sensorCount <= 0)
        return;

    const uint8_t textSize = sensorCount <= 2 ? 2 : 1;
    const int lineHeight = sensorCount <= 2 ? 18 : 9;
    const int baseY = 0;
    const int textX = 10;

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

