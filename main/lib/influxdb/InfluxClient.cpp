#include "InfluxClient.h"
#include "esp_log.h"
#include <cstring>
#include <cstdio>

static const char* TAG = "InfluxClient";

InfluxClient::InfluxClient(const char* baseUrl,
                           const char* apiKey,
                           const char* organization,
                           const char* bucket,
                           const char* precision)
    : _client(nullptr),
      _stream(),
      _baseUrl(baseUrl),
      _apiKey(apiKey),
      _organization(organization),
      _bucket(bucket),
      _precision(precision)
{
    _urlBuffer[0] = '\0';
}

InfluxClient::~InfluxClient()
{
    if (_client) {
        esp_http_client_cleanup(_client);
        _client = nullptr;
    }
}

void InfluxClient::beginRequest()
{
    std::snprintf(_urlBuffer, sizeof(_urlBuffer),
                  "%s?org=%s&bucket=%s&precision=%s",
                  _baseUrl, _organization, _bucket, _precision);

    esp_http_client_config_t config = {};
    config.url = _urlBuffer;
    config.method = HTTP_METHOD_POST;
    config.timeout_ms = 5000;
    config.disable_auto_redirect = true;

    _client = esp_http_client_init(&config);
    if (!_client) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        return;
    }

    ESP_LOGI(TAG, "Opening HTTP connection to: %s", _urlBuffer);

    esp_http_client_set_header(_client, "Content-Type", "text/plain; charset=utf-8");

    if (_apiKey && std::strlen(_apiKey) > 0) {
        char authHeader[128];
        std::snprintf(authHeader, sizeof(authHeader), "Token %s", _apiKey);
        esp_http_client_set_header(_client, "Authorization", authHeader);
    }

    esp_http_client_set_header(_client, "Transfer-Encoding", "chunked");

    esp_err_t err = esp_http_client_open(_client, -1);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        esp_http_client_cleanup(_client);
        _client = nullptr;
        return;
    }

    _stream = RequestStream(_client);
}


void InfluxClient::endRequest()
{
    if (!_client) return;

    _stream.close();

    esp_err_t err = esp_http_client_perform(_client);
    int status = esp_http_client_get_status_code(_client);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "HTTP perform failed: %s (%d)", esp_err_to_name(err), err);
    }
    else
    {
        ESP_LOGI(TAG, "HTTP status: %d", status);
    }

    // Try to read any response body (Influx may send JSON error)
    char buf[128];
    int read_len = esp_http_client_read(_client, buf, sizeof(buf) - 1);
    if (read_len > 0)
    {
        buf[read_len] = '\0';
        ESP_LOGW(TAG, "Response: %s", buf);
    }
    else
    {
        ESP_LOGI(TAG, "No response body received.");
    }

    // Additional internal state info
    int64_t content_length = esp_http_client_get_content_length(_client);
    ESP_LOGI(TAG, "Content length: %lld", content_length);

    // Clean up
    esp_http_client_close(_client);
    esp_http_client_cleanup(_client);
    _client = nullptr;
}


InfluxMeasurementStreamWriter InfluxClient::Measurement(const char* name, time_t timestamp)
{
    beginRequest();
    return InfluxMeasurementStreamWriter(_stream, *this, name, static_cast<int64_t>(timestamp));
}
