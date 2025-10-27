#pragma once
#include "ServiceProvider.h"
#include "InitGuard.h"
#include "RecursiveMutex.h"
#include "TickContext.h"
#include "SettingsManager.h"
#include "esp_log.h"
#include "HttpClientRequest.h"

struct Manifest
{
    char sha256[65] = {}; // 64 + null terminator
};


class WebUpdateManager
{
    inline static constexpr const char *TAG = "WebUpdateManager";
    inline static constexpr Milliseconds CHECK_INTERVAL = 10000;//3600000; // 1 hour

public:
    explicit WebUpdateManager(ServiceProvider &ctx);

    void Init();
    void Tick(TickContext &ctx);

private:
    SettingsManager &settingsManager;
    InitGuard initGuard;
    RecursiveMutex mutex;
    Milliseconds lastCheck = 0;
    HttpClient client;
    char currentSha[65] = {};

    void DownloadIfRequired();
    bool GetManifest(const char* manifestUrl, Manifest& out_manifest);
    bool DownloadWebBundle(const char* webbundleUrl);

};
