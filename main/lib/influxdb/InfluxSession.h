#pragma once
#include "HttpClient.h"
#include "HttpRequest.h"
#include "HttpRequestStream.h"
#include "DateTime.h"

class InfluxSession {
public:
    InfluxSession(const char* url,
                  const char* apiKey,
                  const char* measurementName,
                  const DateTime& timestamp,
                  TickType_t timeout);

    ~InfluxSession();

    bool isValid() const { return _valid; }

    // --- Tags ---
    InfluxSession& withTag(const char* key, const char* value);

    // --- Fields ---
    InfluxSession& withField(const char* key, float value);

    // --- New measurement ---
    InfluxSession& withMeasurement(const char* name, const DateTime& timestamp);

    // --- Finish ---
    void Finish();

private:
    HttpRequest _req;
    HttpRequestStream _stream;
    DateTime _timestamp;
    bool _valid;
    bool _hasFields;
};
