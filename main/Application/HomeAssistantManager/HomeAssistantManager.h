#pragma once

#include "ServiceProvider.h"
#include "InitState.h"
#include "Timer.h"

class HomeAssistantManager
{
    static constexpr const char *TAG = "HomeAssistantManager";

public:
    explicit HomeAssistantManager(ServiceProvider &serviceProvider);

    HomeAssistantManager(const HomeAssistantManager &) = delete;
    HomeAssistantManager &operator=(const HomeAssistantManager &) = delete;
    HomeAssistantManager(HomeAssistantManager &&) = delete;
    HomeAssistantManager &operator=(HomeAssistantManager &&) = delete;

    void Init();

    void PublishTemperatures();

private:
    ServiceProvider &serviceProvider_;
    InitState initState_;

    static constexpr int MAX_SENSORS = 4;
    static constexpr const char *SLOT_NAMES[] = {"Red", "Blue", "Green", "Yellow"};

    Timer publishTimer_;

    void PublishLedState();
};
