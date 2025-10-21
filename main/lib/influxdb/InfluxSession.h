#pragma once
#include "HttpClient.h"
#include "HttpRequest.h"
#include "HttpRequestStream.h"
#include "DateTime.h"
#include "StringWriter.h"

class InfluxSession {
public:
    InfluxSession(const char* url,
                  const char* apiKey,
                  const char* measurementName,
                  const DateTime& timestamp,
                  TickType_t timeout);

    ~InfluxSession();

    InfluxSession& withTag(const char* key, const char* value);
    InfluxSession& withField(const char* key, float value);
    InfluxSession& withField(const char* key, double value);
    InfluxSession& withField(const char* key, int32_t value);
    InfluxSession& withField(const char* key, bool value);

    InfluxSession& withMeasurement(const char* name, const DateTime& timestamp);

    bool Finish();

private:
    HttpRequest _req;
    HttpRequestStream _stream;
    StringWriter _writer; 
    DateTime _timestamp;
    bool _hasFields;
    bool _open;
};
