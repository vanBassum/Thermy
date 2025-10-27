#pragma once
#include "HttpClientRequest.h"
#include "DateTime.h"
#include "StringWriter.h"
#include "InitGuard.h"
#include "esp_log.h"
#include "BufferedStream.h"

class InfluxSession
{
    inline static constexpr const char* TAG = "InfluxSession";

    enum class WritePhase {
        None,
        Measurement,
        Tags,
        Fields
    };

public:
    InfluxSession(HttpClient& client,
                  const char* endpoint,
                  const char* apiKey,
                  TickType_t timeout);
    ~InfluxSession();

    // Disable copy
    InfluxSession(const InfluxSession&) = delete;
    InfluxSession& operator=(const InfluxSession&) = delete;


    InfluxSession& withMeasurement(const char* name, const DateTime& timestamp);
    InfluxSession& withTag(const char* key, const char* value);
    InfluxSession& withField(const char* key, float value);
    InfluxSession& withField(const char* key, int32_t value);
    InfluxSession& withField(const char* key, bool value);

    bool Finish();

private:
    HttpClientRequest _req;
    DateTime _timestamp;
    InitGuard _initGuard;
    WritePhase _phase = WritePhase::None;
    BufferedStream<256> _bufferedStream;

    void WriteEscaped(const char* text);
};
