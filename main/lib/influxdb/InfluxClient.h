#pragma once
#include "HttpClient.h"
#include "InfluxStreamWriter.h"
#include <cstdint>

class InfluxClient {
public:
    InfluxClient(const char* baseUrl,
                 const char* apiKey,
                 const char* organization,
                 const char* bucket,
                 const char* precision = "s");

    InfluxStreamWriter Measurement(const char* name, const DateTime& timestamp, TickType_t timeout = pdMS_TO_TICKS(5000));

private:
    const char* _baseUrl;
    const char* _apiKey;
    const char* _organization;
    const char* _bucket;
    const char* _precision;

    char _urlBuffer[256];
};

class NullStream : public Stream {
public:
    size_t write(const void*, size_t len) override { return len; }
    size_t read(void*, size_t) override { return 0; }
    void flush() override {}
};
