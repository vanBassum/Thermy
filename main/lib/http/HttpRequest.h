#pragma once
#include "freertos/FreeRTOS.h"
#include "esp_http_client.h" 
#include "HttpRequestStream.h"
#include <cstddef>

class HttpRequest {
public:
    HttpRequest(const char* url, esp_http_client_method_t method, TickType_t timeoutTicks);
    ~HttpRequest();

    bool open();
    void close();

    int getStatusCode() const;
    int64_t getContentLength() const;

    void setHeader(const char* key, const char* value);
    HttpRequestStream createStream();

private:
    esp_http_client_handle_t _client;
    esp_http_client_config_t _config;
    bool _opened;
    TickType_t _timeoutTicks;
};
