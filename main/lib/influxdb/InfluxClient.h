#pragma once
#include "HttpClient.h"
#include "InfluxSession.h"
#include "DateTime.h"
#include <cstdio>

class InfluxClient {
public:
    InfluxClient(const char* baseUrl,
                 const char* apiKey,
                 const char* organization,
                 const char* bucket)
        : _apiKey(apiKey)
    {
        std::snprintf(_urlBuffer, sizeof(_urlBuffer),
                      "%s?org=%s&bucket=%s&precision=s",
                      baseUrl, organization, bucket);
    }

    // Starts a new write session
    InfluxSession Measurement(const char* name, const DateTime& timestamp, TickType_t timeout)
    {
        return InfluxSession(_urlBuffer, _apiKey, name, timestamp, timeout);
    }

private:
    const char* _apiKey;
    char _urlBuffer[256];
};
