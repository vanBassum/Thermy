#pragma once
#include "ServiceProvider.h"
#include "InitGuard.h"
#include "RecursiveMutex.h"
#include "TickContext.h"
#include "SettingsManager.h"
#include "esp_log.h"

class WebUpdateManager
{
    inline static constexpr const char *TAG = "WebUpdateManager";
    inline static constexpr Milliseconds CHECK_INTERVAL = 3600000; // 1 hour

public:
    explicit WebUpdateManager(ServiceProvider &ctx);

    void Init();
    void Tick(TickContext &ctx);

private:
    SettingsManager &settingsManager;
    InitGuard initGuard;
    RecursiveMutex mutex;

    Milliseconds lastCheck = 0;

    bool FetchManifest(std::string &jsonOut);
    bool ParseManifest(const std::string &json, std::string &remoteHash);
    bool ReadLocalHash(std::string &hashOut);
    bool WriteLocalManifest(const std::string &json);
    bool HashChanged(const std::string &local, const std::string &remote);
    bool DownloadAndExtractBundle();
};
