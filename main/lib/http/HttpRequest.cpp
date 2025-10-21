#include "HttpRequest.h"
#include "esp_log.h"
#include "esp_err.h"
#include <cstring>

static const char* TAG = "HttpRequest";

HttpRequest::HttpRequest(const char* url, esp_http_client_method_t method, TickType_t timeoutTicks)
    : _client(nullptr),
      _opened(false),
      _timeoutTicks(timeoutTicks)
{
    memset(&_config, 0, sizeof(_config));
    _config.url = url;
    _config.method = method;

    // Convert ticks → ms safely
    int timeout_ms = (timeoutTicks * 1000) / configTICK_RATE_HZ;
    _config.timeout_ms = timeout_ms;
}

HttpRequest::~HttpRequest()
{
    close();
}

bool HttpRequest::open()
{
    _client = esp_http_client_init(&_config);
    if (!_client) {
        ESP_LOGE(TAG, "Failed to init HTTP client");
        return false;
    }

    esp_err_t err = esp_http_client_open(_client, -1);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        esp_http_client_cleanup(_client);
        _client = nullptr;
        return false;
    }

    _opened = true;
    return true;
}

void HttpRequest::close()
{
    if (_client) {
        esp_http_client_close(_client);
        esp_http_client_cleanup(_client);
        _client = nullptr;
        _opened = false;
    }
}

int HttpRequest::getStatusCode() const
{
    return _client ? esp_http_client_get_status_code(_client) : -1;
}

int64_t HttpRequest::getContentLength() const
{
    return _client ? esp_http_client_get_content_length(_client) : 0;
}

void HttpRequest::setHeader(const char* key, const char* value)
{
    if (_client) {
        esp_http_client_set_header(_client, key, value);
    }
}

HttpRequestStream HttpRequest::createStream()
{
    return HttpRequestStream(_client);
}
