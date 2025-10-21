#pragma once
#include "freertos/FreeRTOS.h"
#include "esp_http_client.h" 
#include "HttpRequest.h"
#include <cstddef>

class HttpClient {
    constexpr static const char* TAG = "HttpClient";
public:
    HttpClient();
    ~HttpClient();

    // Explicitly requires HTTP method and FreeRTOS tick timeout
    HttpRequest createRequest(const char* url, esp_http_client_method_t method, TickType_t timeout);

};
