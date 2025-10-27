#pragma once
#include "esp_http_client.h"
#include "InitGuard.h"

class HttpClient {
public:
    HttpClient() = default;
    ~HttpClient();

    bool Init(const char* baseUrl);
    esp_http_client_handle_t GetHandle() const { REQUIRE_READY(initGuard); return _handle; }
    const char* GetBaseUrl() const { REQUIRE_READY(initGuard); return _baseUrl; }

private:
    InitGuard initGuard;
    const char* _baseUrl = nullptr;
    esp_http_client_handle_t _handle = nullptr;
};
