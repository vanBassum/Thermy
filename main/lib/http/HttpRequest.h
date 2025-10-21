#pragma once
#include "HttpRequestStream.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include <cstring>

class HttpRequest
{
    inline static constexpr const char* TAG = "HttpRequest";
public:
    HttpRequest() = default;
    ~HttpRequest();

    void Init(const char* url, esp_http_client_method_t method, TickType_t timeoutTicks);

    bool Open();
    void Close();

    void SetHeader(const char* key, const char* value);
    int GetStatusCode() const;

    HttpRequestStream& Stream() { return _stream; }

    esp_http_client_handle_t GetClientHandle() const { return _client; }

    int Perform();

private:
    esp_http_client_config_t _config{};
    esp_http_client_handle_t _client = nullptr;
    bool _opened = false;
    HttpRequestStream _stream;  // persistent stream for this request
};
