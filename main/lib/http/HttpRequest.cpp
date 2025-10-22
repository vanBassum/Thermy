#include "HttpRequest.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"

HttpRequest::~HttpRequest()
{
    Close();
}

void HttpRequest::Init(const char *url, esp_http_client_method_t method, TickType_t timeoutTicks)
{
    memset(&_config, 0, sizeof(_config));
    _config.url = url;
    _config.method = method;
    _config.timeout_ms = pdTICKS_TO_MS(timeoutTicks);
    _config.crt_bundle_attach = esp_crt_bundle_attach;

    // 💡 Initialize client immediately so headers can be set afterward
    _client = esp_http_client_init(&_config);
    if (!_client) {
        ESP_LOGE(TAG, "Failed to init HTTP client");
        return;
    }
}

bool HttpRequest::Open()
{
    if (_opened)
        return true;

    // Tell the server we’ll stream the body in chunks
    esp_http_client_set_header(_client, "Transfer-Encoding", "chunked");

    esp_err_t err = esp_http_client_open(_client, -1);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to open HTTP request: %s", esp_err_to_name(err));
        esp_http_client_cleanup(_client);
        _client = nullptr;
        return false;
    }

    _stream.Init(_client, true);
    _opened = true;
    return true;
}

// HttpRequest.cpp
void HttpRequest::Close()
{
    if (!_opened)
        return;

    _stream.close();
    esp_http_client_close(_client);
    esp_http_client_cleanup(_client);

    _client = nullptr;
    _opened = false;
}


void HttpRequest::SetHeader(const char* key, const char* value)
{
    if (_client)
        esp_http_client_set_header(_client, key, value);
}

int HttpRequest::GetStatusCode() const
{
    if (!_client)
        return -1;
    return esp_http_client_get_status_code(_client);
}

int HttpRequest::Perform()
{
    if (!_opened || !_client)
        return -1;

    esp_err_t err = esp_http_client_perform(_client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "HTTP perform failed: %s", esp_err_to_name(err));
        return -1;
    }

    int status = esp_http_client_get_status_code(_client);

    if (status >= 400) {
        char buf[256];
        int n = esp_http_client_read(_client, buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';
            ESP_LOGW(TAG, "HTTP error response: %s", buf);
        } else {
            ESP_LOGW(TAG, "HTTP error: no response body");
        }
    }

    return status;
}