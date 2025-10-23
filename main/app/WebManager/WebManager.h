#pragma once
#include "ServiceProvider.h"
#include "rtos.h"
#include "DateTime.h"
#include "TickContext.h"

class WebManager
{
    inline static constexpr const char *TAG = "WebManager";

public:
    explicit WebManager(ServiceProvider &ctx);
    ~WebManager();

    void Init();
    void Tick(TickContext& ctx);

private:
    InitGuard initGuard;
    RecursiveMutex mutex;
};
