#pragma once
#include "ServiceProvider.h"
#include "rtos.h"
#include "DateTime.h"
#include "TickContext.h"
#include "HttpWebServer.h"
#include "TestEndpoint.h"
#include "FileGetEndpoint.h"
#include "GetTemperaturesEndpoint.h"

class WebManager
{
    inline static constexpr const char *TAG = "WebManager";

public:
    explicit WebManager(ServiceProvider &ctx)
        : ctx(ctx)
    {
    }
    ~WebManager() = default;

    void Init()
    {
        server.start();
        server.registerEndpoint("/test", HTTP_GET, testEndpoint);
        server.registerEndpoint("/api/temperatures", HTTP_GET, getTemperaturesEndpoint);
        server.registerEndpoint("/*", HTTP_GET, fileGetEndpoint);
        server.EnableCors();
    }

    void Tick(TickContext &ctx)
    {
    }

private:
    ServiceProvider &ctx;

    InitGuard initGuard;
    RecursiveMutex mutex;

    HttpWebServer server;
    TestEndpoint testEndpoint;
    FileGetEndpoint fileGetEndpoint{"/fat"};
    GetTemperaturesEndpoint getTemperaturesEndpoint{ctx};
};
