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
    ESP_LOGI(TAG, "HTTP client initialized for URL: %s", url);
}

bool HttpClientRequest::Open()
{
    assert(_client && "Client not initialized");
    assert(!_opened && "Request already opened");

    SetHeader("Transfer-Encoding", "chunked");

    esp_err_t err = esp_http_client_open(_client, -1);
    assert(err == ESP_OK && "esp_http_client_open failed");


    ESP_LOGI(TAG, "HTTP request opened");   
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
    esp_http_client_write(_client, "0\r\n\r\n", 5);

    esp_err_t err = esp_http_client_perform(_client);
    if (err != ESP_OK)
    {
        // Distinguish common cases (do NOT read the body here!)
        if (err == ESP_ERR_HTTP_EAGAIN) {
            ESP_LOGW(TAG, "HTTP perform timeout (ESP_ERR_HTTP_EAGAIN)");
            return -2; // timeout / try again
        }
        if (err == ESP_ERR_NOT_SUPPORTED) {
            ESP_LOGE(TAG, "HTTP perform failed: Authentication challenge not supported (ESP_ERR_NOT_SUPPORTED)");
            return -3; // auth challenge we didn't satisfy (e.g., 401 with WWW-Authenticate)
        }

        ESP_LOGE(TAG, "HTTP perform failed: %s", esp_err_to_name(err));
        return -1; // generic transport/protocol failure
    }

    // Only now is it safe to query status and possibly read body
    int status = esp_http_client_get_status_code(_client);

    // Treat invalid/missing status as error
    if (status <= 0) {
        ESP_LOGE(TAG, "No valid HTTP status after perform");
        return -1;
    }

    // HTTP error classes (4xx/5xx): body may or may not be available; read is safe now
    if (status >= 400) {
        char buf[256];
        int n = esp_http_client_read(_client, buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';
            ESP_LOGW(TAG, "HTTP error response (%d): %s", status, buf);
        } else {
            ESP_LOGW(TAG, "HTTP error (%d): no response body", status);
        }
        return status; // return the actual HTTP status so caller can branch on 401, 429, 5xx, etc.
    }

    // Non-2xx success (e.g. 202) is often fine; log for visibility
    if (status < 200 || status > 299) {
        ESP_LOGW(TAG, "Unexpected HTTP status: %d", status);
    }

    return status;
}
