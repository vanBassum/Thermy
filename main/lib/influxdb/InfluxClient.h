#pragma once
#include <ctime>
#include <cstdint>
#include "InfluxMeasurementStreamWriter.h"
#include "RequestStream.h"
#include "esp_http_client.h"

// InfluxDB client that owns esp_http_client and manages the request stream.
class InfluxClient {
public:
    // precision: "s", "ms", or "ns" (default = "s" for time_t)
    InfluxClient(const char* baseUrl,
                 const char* apiKey,
                 const char* organization,
                 const char* bucket,
                 const char* precision = "s");

    ~InfluxClient();

    // Fluent entrypoint using time_t directly
    InfluxMeasurementStreamWriter Measurement(const char* name, time_t timestamp);

private:
    friend class InfluxMeasurementStreamWriter;

    esp_http_client_handle_t _client;
    RequestStream _stream;

    const char* _baseUrl;
    const char* _apiKey;
    const char* _organization;
    const char* _bucket;
    const char* _precision;

    char _urlBuffer[256];

    void beginRequest();
    void endRequest();
};
