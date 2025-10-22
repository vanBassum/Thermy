#include "DisplayManager.h"
#include "WifiManager.h"
#include "esp_log.h"
#include "esp_netif.h"

DisplayManager::DisplayManager(ServiceProvider &ctx)
    : sensorManager(ctx.GetSensorManager())
    , wifiManager(ctx.GetWifiManager())
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

    DrawHeader(display);
    DrawSensorTemperatures(display);

    display.show();
}

void DisplayManager::DrawHeader(SSD1306 &display)
{
    TextStyle iconStyle(&Font8x8_Symbols, 1, true);
    TextStyle timeStyle(&Font5x7, 1, true);

    // --- Draw WiFi Icon (top-left)
    char wifiChar = wifiManager.IsConnected() ? (char)SymbolIcon::Wifi : ' ';
    display.drawChar(0, 0, wifiChar, iconStyle);

    // --- Draw time (top-right)
    char timeStr[9];
    DateTime::Now().ToStringLocal(timeStr, sizeof(timeStr), "%H:%M:%S");

    int textWidth = (Font5x7.width + 1) * strlen(timeStr);
    int x = display.getWidth() - textWidth;
    display.drawText(x, 0, timeStr, timeStyle);
}

void DisplayManager::DrawSensorTemperatures(SSD1306 &display)
{
    const int sensorCount = sensorManager.GetSensorCount();

    if (sensorCount <= 0)
        return;

    uint8_t textSize = 1;
    int lineHeight = 10;

    if (sensorCount == 1)
    {
        textSize = 3;
        lineHeight = 24;
    }
    else if (sensorCount == 2)
    {
        textSize = 2;
        lineHeight = 16;
    }

    TextStyle tempStyle(&Font5x7, textSize, true);
    const int baseY = 12; // below header

    for (int i = 0; i < sensorCount; ++i)
    {
        float tempC = sensorManager.GetTemperature(i);

        char line[16];
        snprintf(line, sizeof(line), "%.1f°C", tempC);

        int y = baseY + i * lineHeight;
        display.drawText(0, y, line, tempStyle);
    }
}
