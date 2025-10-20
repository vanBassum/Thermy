#pragma once
#include "RootSettings.h"
#include "rtos.h"
#include <esp_log.h>
#include "ServiceProvider.h"
#include "InitGuard.h"

class SettingsManager
{
    constexpr static const char *TAG = "SettingsManager";
    constexpr static const char *NVS_PARTITION = "nvs";

public:
    explicit SettingsManager(ServiceProvider& services);
    SettingsManager(const SettingsManager &) = delete;
    SettingsManager &operator=(const SettingsManager &) = delete;
    SettingsManager(SettingsManager &&) = default;
    SettingsManager &operator=(SettingsManager &&) = default;

    void Init();

    template <typename FUNC>
    void Access(FUNC func)
    {
        LOCK(mutex);
        func(rootSettings);
    }

    // ---- Load / Save ----
    void LoadAll();
    void SaveAll();

    void LoadGroup(ISettingsGroup &group);
    void SaveGroup(const ISettingsGroup &group);

    // ---- Reset ----
    void ResetToFactoryDefaults();
    void ResetGroupToFactoryDefaults(ISettingsGroup &group);

    // ---- Utility ----
    void Print(ISettingsGroup &group);
    bool SetValueByString(const char* path, const char* value);
    bool GetValueAsString(const char* path, char* outBuf, size_t outBufSize) const;

private:
    RootSettings rootSettings;
    RecursiveMutex mutex;
    InitGuard initGuard;

    const char* FindGroupName(const ISettingsGroup* groupPtr) const;
};
