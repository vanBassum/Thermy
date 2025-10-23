#include "WebManager.h"

WebManager::WebManager(ServiceProvider &ctx)
{
}

WebManager::~WebManager()
{
}

void WebManager::Init()
{
    server.start();
    server.registerEndpoint("/test", HTTP_GET, testEndpoint);
    server.registerEndpoint("/*", HTTP_GET, fileGetEndpoint);
    server.EnableCors();
}

void WebManager::Tick(TickContext &ctx)
{
}
