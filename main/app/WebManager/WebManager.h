#pragma once
#include "ServiceProvider.h"
#include "rtos.h"
#include "DateTime.h"
#include "HttpWebServer.h"
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
        //server.registerEndpoint("/test", HTTP_GET, testEndpoint);
        server.registerEndpoint("/api/temperatures", HTTP_GET, getTemperaturesEndpoint);
        //server.registerEndpoint("/api/stats", HTTP_GET, getStatsEndpoint);

        server.registerEndpoint("/*", HTTP_GET, fileGetEndpoint);
        server.EnableCors();
    }

private:
    ServiceProvider &ctx;

    InitGuard initGuard;
    RecursiveMutex mutex;

    HttpWebServer server;
    FileGetEndpoint fileGetEndpoint{"/fat"};
    GetTemperaturesEndpoint getTemperaturesEndpoint{ctx};
};
