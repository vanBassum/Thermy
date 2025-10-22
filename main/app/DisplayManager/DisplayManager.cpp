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

    SSD1306& display = driver.GetDisplay();

    // --- Clear screen ---
    display.fill(0);

    // --- Draw header (WiFi + Time) ---
    char headerLine[24];
    char timeStr[6];
    DateTime::Now().ToStringLocal(timeStr, sizeof(timeStr), "%H:%M");

    snprintf(headerLine, sizeof(headerLine), "%-4s  %s",
             wifiManager.IsConnected() ? "WiFi" : "    ",
             timeStr);

    // Small font for header
    display.drawText(0, 0, headerLine, 1);

    // --- Determine sensor text size and spacing ---
    const int sensorCount = sensorManager.GetSensorCount();
    int textSize;
    int lineHeight;

    if (sensorCount <= 1) {
        textSize = 3;
        lineHeight = 24;
    } else if (sensorCount == 2) {
        textSize = 2;
        lineHeight = 16;
    } else {
        textSize = 1;
        lineHeight = 10;
    }

    // --- Draw temperatures ---
    const int baseY = 12;  // Start below header
    for (int i = 0; i < sensorCount; ++i)
    {
        float tempC = sensorManager.GetTemperature(i);

        char line[16];
        snprintf(line, sizeof(line), "%.1f", tempC);

        // Vertical placement based on lineHeight
        int y = baseY + i * lineHeight;
        display.drawText(0, y, line, textSize);
    }

    // --- Commit frame ---
    display.show();
}
