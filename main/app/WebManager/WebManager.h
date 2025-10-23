#pragma once
#include "ServiceProvider.h"
#include "rtos.h"
#include "DateTime.h"
#include "TickContext.h"
#include "HttpWebServer.h"
#include "TestEndpoint.h"
#include "FileGetEndpoint.h"

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

    HttpWebServer server;
    TestEndpoint testEndpoint;
    FileGetEndpoint fileGetEndpoint{"/fat"};

};
