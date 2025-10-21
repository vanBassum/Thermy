#pragma once
#include "HttpRequest.h"
#include "HttpRequestStream.h"
#include "DateTime.h"
#include "StringWriter.h"
#include "InitGuard.h"
#include "esp_log.h"

class InfluxSession
{
    inline static constexpr const char* TAG = "InfluxSession";

public:
    InfluxSession() = default;
    ~InfluxSession();

    // Disable copy
    InfluxSession(const InfluxSession&) = delete;
    InfluxSession& operator=(const InfluxSession&) = delete;

    // Enable move
    InfluxSession(InfluxSession&& other) noexcept;
    InfluxSession& operator=(InfluxSession&& other) noexcept;

    /// Initializes the session.
    void Init(const char* url,
              const char* apiKey,
              const char* measurementName,
              const DateTime& timestamp,
              TickType_t timeout);

    InfluxSession& withTag(const char* key, const char* value);
    InfluxSession& withField(const char* key, float value);
    InfluxSession& withField(const char* key, int32_t value);
    InfluxSession& withField(const char* key, bool value);

    InfluxSession& withMeasurement(const char* name, const DateTime& timestamp);

    bool Finish();

private:
    HttpRequest _req;
    DateTime _timestamp;
    InitGuard _initGuard;

    void WriteEscaped(const char* text);
};
