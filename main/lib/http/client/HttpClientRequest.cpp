#include "HttpClientRequest.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include <cassert>

HttpClientRequest::~HttpClientRequest()
{
    Close();
}

void HttpClientRequest::Init(const char *url, esp_http_client_method_t method, TickType_t timeoutTicks)
{
    assert(url);
    assert(!_client && "Client already initialized");

    memset(&_config, 0, sizeof(_config));
    _config.url = url;
    _config.method = method;
    _config.timeout_ms = pdTICKS_TO_MS(timeoutTicks);
    _config.crt_bundle_attach = esp_crt_bundle_attach;

    _client = esp_http_client_init(&_config);
    assert(_client && "esp_http_client_init failed");
}

bool HttpClientRequest::Open()
{
    assert(_client && "Client not initialized");
    assert(!_opened && "Request already opened");

    SetHeader("Transfer-Encoding", "chunked");

    esp_err_t err = esp_http_client_open(_client, -1);
    assert(err == ESP_OK && "esp_http_client_open failed");

    _stream.Init(_client, true);
    _opened = true;
    return true;
}

void HttpClientRequest::Close()
{
    if (!_client)
        return;

    if (_opened)
        esp_http_client_close(_client);

    esp_http_client_cleanup(_client);
    _stream.Init(nullptr, false);
    _client = nullptr;
    _opened = false;
}

void HttpClientRequest::SetHeader(const char *key, const char *value)
{
    assert(_client && "Client not initialized");
    assert(!_opened && "Request already opened");
    esp_http_client_set_header(_client, key, value);
}

int HttpClientRequest::GetStatusCode() const
{
    assert(_client && "Client not initialized");
    assert(_opened && "Request not opened");
    return esp_http_client_get_status_code(_client);
}

int HttpClientRequest::Perform()
{
    assert(_client && "Client not initialized");
    assert(_opened && "Request not opened");

    _stream.flush();

    esp_err_t err = esp_http_client_perform(_client);
    if (err != ESP_OK)
    {
        if (err == ESP_ERR_HTTP_EAGAIN)
        {
            ESP_LOGW(TAG, "HTTP perform timeout (ESP_ERR_HTTP_EAGAIN)");
            return -2; // special case for timeout
        }

        ESP_LOGE(TAG, "HTTP perform failed: %s", esp_err_to_name(err));
        char buf[128];
        int n = esp_http_client_read(_client, buf, sizeof(buf) - 1);
        if (n > 0)
        {
            buf[n] = '\0';
            ESP_LOGE(TAG, "Partial error response: %s", buf);
        }
        return -1;
    }

    int status = esp_http_client_get_status_code(_client);

    if (status <= 0 || status >= 400)
    {
        char buf[256];
        int n = esp_http_client_read(_client, buf, sizeof(buf) - 1);
        if (n > 0)
        {
            buf[n] = '\0';
            ESP_LOGW(TAG, "HTTP error response (%d): %s", status, buf);
        }
        else
        {
            ESP_LOGW(TAG, "HTTP error (%d): no response body", status);
        }
        return -1;
    }

    if (status < 200 || status > 299)
    {
        ESP_LOGW(TAG, "Unexpected HTTP status: %d", status);
    }

    return status;
}
