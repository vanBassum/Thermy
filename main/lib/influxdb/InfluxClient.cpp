#include "InfluxClient.h"
#include "esp_log.h"
#include <cstdio>
#include <cstring>

static const char* TAG = "InfluxClient";

InfluxClient::InfluxClient(const char* baseUrl,
                           const char* apiKey,
                           const char* organization,
                           const char* bucket,
                           const char* precision)
    : _baseUrl(baseUrl),
      _apiKey(apiKey),
      _organization(organization),
      _bucket(bucket),
      _precision(precision)
{
    _urlBuffer[0] = '\0';
}


InfluxStreamWriter InfluxClient::Measurement(const char* name, const DateTime& timestamp, TickType_t timeout)
{
    std::snprintf(_urlBuffer, sizeof(_urlBuffer),
                  "%s?org=%s&bucket=%s&precision=%s",
                  _baseUrl, _organization, _bucket, _precision);

    ESP_LOGI(TAG, "Opening Influx write to: %s", _urlBuffer);

    HttpClient http;
    HttpRequest req = http.createRequest(_urlBuffer, HTTP_METHOD_POST, timeout);

    req.setHeader("Content-Type", "text/plain; charset=utf-8");
    req.setHeader("Transfer-Encoding", "chunked");

    if (_apiKey && std::strlen(_apiKey) > 0) {
        char authHeader[128];
        std::snprintf(authHeader, sizeof(authHeader), "Token %s", _apiKey);
        req.setHeader("Authorization", authHeader);
    }

    if (!req.open()) {
        ESP_LOGE(TAG, "Failed to open Influx write connection");
        static NullStream nullStream;
        return InfluxStreamWriter(nullStream, name, timestamp);
    }

    HttpRequestStream stream = req.createStream();
    return InfluxStreamWriter(stream, name, timestamp);
}



