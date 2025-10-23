#pragma once
#include "HttpClientRequestStream.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include <cstring>
#include "BufferedStream.h"

class HttpClientRequest
{
    inline static constexpr const char* TAG = "HttpClientRequest";
public:
    HttpClientRequest() = default;
    ~HttpClientRequest();

    void Init(const char* url, esp_http_client_method_t method, TickType_t timeoutTicks);
    bool Open();
    void Close();

    void SetHeader(const char* key, const char* value);
    int GetStatusCode() const;

    Stream& GetStream() { return _stream; }
    int Perform();

private:
    esp_http_client_config_t _config{};
    esp_http_client_handle_t _client = nullptr;
    bool _opened = false;
    HttpClientRequestStream _stream;  // persistent stream for this request
};
