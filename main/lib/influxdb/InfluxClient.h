#pragma once
#include "InfluxSession.h"
#include "DateTime.h"
#include <cstdio>
#include <cstring>

class InfluxClient
{
public:
    InfluxClient() = default;

    void Init(const char* baseUrl,
              const char* apiKey,
              const char* organization,
              const char* bucket)
    {
        // Allow re-initialization
        _initGuard.SetNotReady();

        std::strncpy(_apiKey, apiKey ? apiKey : "", sizeof(_apiKey) - 1);
        _apiKey[sizeof(_apiKey) - 1] = '\0';

        std::snprintf(_urlBuffer, sizeof(_urlBuffer),
                      "%s?org=%s&bucket=%s&precision=s",
                      baseUrl ? baseUrl : "",
                      organization ? organization : "",
                      bucket ? bucket : "");

        _initGuard.SetReady();
    }

    /// Starts a new write session
    InfluxSession Measurement(const char* name, const DateTime& timestamp, TickType_t timeout)
    {
        REQUIRE_READY(_initGuard);
        InfluxSession session;
        session.Init(_urlBuffer, _apiKey, name, timestamp, timeout);
        return session;
    }

private:
    InitGuard _initGuard;
    char _apiKey[128] = {};
    char _urlBuffer[256] = {};
};
