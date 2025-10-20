#pragma once
#include "ServiceProvider.h"
#include "InitGuard.h"
#include "Mutex.h"
#include "esp_log.h"

// Later you’ll replace this with your actual display driver class
// For now we’ll mock the interface
class DisplayDriver {
public:
    void Init() {
        ESP_LOGI("DisplayDriver", "Initializing mock display...");
    }
    void Clear() {
        ESP_LOGI("DisplayDriver", "Clearing display");
    }
    void PrintLine(int line, const char *text) {
        ESP_LOGI("DisplayDriver", "Line %d: %s", line, text);
    }
};

class DisplayManager
{
    inline static constexpr const char *TAG = "DisplayManager";

public:
    explicit DisplayManager(ServiceProvider &ctx);

    void Init();
    void Loop();

    void Clear();
    void PrintLine(int line, const char *text);
    void ShowSplashScreen();

    bool IsInitialized() const { return initGuard.IsReady(); }

private:
    ServiceProvider &_ctx;
    InitGuard initGuard;
    Mutex mutex;
    DisplayDriver driver;

    uint32_t lastUpdate = 0;
};
