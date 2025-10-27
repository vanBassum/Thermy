#pragma once
#include "InfluxSession.h"
#include "HttpClient.h"
#include <cstdio>
#include <cstring>

class InfluxClient
{
public:
    InfluxClient() = default;

    void Init(const char* baseUrl, // e.g. "http://influxdb.local:8086"
              const char* apiKey,
              const char* organization,
              const char* bucket)
    {
        _initGuard.SetNotReady();

        std::strncpy(_apiKey, apiKey ? apiKey : "", sizeof(_apiKey) - 1);
        _apiKey[sizeof(_apiKey) - 1] = '\0';

        std::snprintf(_endpoint, sizeof(_endpoint),
                      "/api/v2/write?org=%s&bucket=%s&precision=s",
                      organization ? organization : "",
                      bucket ? bucket : "");

        _httpClient.Init(baseUrl);
        _initGuard.SetReady();
    }

    InfluxSession CreateSession(TickType_t timeout)
    {
        REQUIRE_READY(_initGuard);
        return InfluxSession(_httpClient, _endpoint, _apiKey, timeout);
    }

private:
    HttpClient _httpClient;
    InitGuard _initGuard;
    char _apiKey[128] = {};
    char _endpoint[256] = {};
};
