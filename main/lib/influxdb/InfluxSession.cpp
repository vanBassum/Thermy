#include "InfluxSession.h"
#include "esp_log.h"
#include <cstdio>
#include <cstring>

static const char* TAG = "InfluxSession";

InfluxSession::InfluxSession(const char* url,
                             const char* apiKey,
                             const char* measurementName,
                             const DateTime& timestamp,
                             TickType_t timeout)
    : _req(nullptr, HTTP_METHOD_POST, timeout),
      _stream(),
      _valid(false),
      _hasFields(false)
{
    ESP_LOGI(TAG, "Starting session: %s", url);

    HttpClient http;
    _req = http.createRequest(url, HTTP_METHOD_POST, timeout);
    _req.setHeader("Content-Type", "text/plain; charset=utf-8");
    _req.setHeader("Transfer-Encoding", "chunked");

    if (apiKey && std::strlen(apiKey) > 0) {
        char auth[128];
        std::snprintf(auth, sizeof(auth), "Token %s", apiKey);
        _req.setHeader("Authorization", auth);
    }

    if (!_req.open()) {
        ESP_LOGE(TAG, "Failed to open HTTP connection");
        return;
    }

    _stream = _req.createStream();
    _valid = true;
    _timestamp = timestamp;

    // Write measurement header
    _stream.write(measurementName, std::strlen(measurementName));
}

InfluxSession::~InfluxSession() {
    Finish();
}

InfluxSession& InfluxSession::withTag(const char* key, const char* value)
{
    if (!_valid) return *this;
    _stream.write(",", 1);
    _stream.write(key, std::strlen(key));
    _stream.write("=", 1);
    _stream.write(value, std::strlen(value));
    return *this;
}

InfluxSession& InfluxSession::withField(const char* key, float value)
{
    if (!_valid) return *this;
    if (!_hasFields) {
        _stream.write(" ", 1);
        _hasFields = true;
    } else {
        _stream.write(",", 1);
    }

    char buf[32];
    int len = std::snprintf(buf, sizeof(buf), "%s=%f", key, value);
    _stream.write(buf, len);
    return *this;
}

InfluxSession& InfluxSession::withMeasurement(const char* name, const DateTime& timestamp)
{
    if (!_valid) return *this;

    // End current line
    char buf[32];
    int len = std::snprintf(buf, sizeof(buf), " %lld\n", _timestamp.UtcSeconds());
    _stream.write(buf, len);

    // Start new measurement
    _stream.write(name, std::strlen(name));
    _timestamp = timestamp;
    _hasFields = false;
    return *this;
}

void InfluxSession::Finish()
{
    if (!_valid) return;

    char buf[32];
    int len = std::snprintf(buf, sizeof(buf), " %lld\n", _timestamp.UtcSeconds());
    _stream.write(buf, len);

    _stream.close();
    _req.close();
    _valid = false;

    ESP_LOGI(TAG, "Session finished");
}
