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

    _client = esp_http_client_init(&_config);
    if (!_client)
        ESP_LOGE(TAG, "Failed to init HTTP client");
    
}

bool HttpRequest::Open()
{
    if (_opened)
        return true;

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
void HttpRequest::Close()
{
    if (!_opened)
        return;

    // Ensure request is performed before closing
    esp_err_t err = esp_http_client_perform(_client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "HTTP perform failed: %s", esp_err_to_name(err));
    }

    _stream.close();
    esp_http_client_close(_client);
    esp_http_client_cleanup(_client);

    _client = nullptr;
    _opened = false;
}

void HttpRequest::SetHeader(const char* key, const char* value)
{
    if (!_client)
        return;

    esp_http_client_set_header(_client, key, value);
}

int HttpRequest::GetStatusCode() const
{
    if (!_client)
        return -1;
    return esp_http_client_get_status_code(_client);
}
