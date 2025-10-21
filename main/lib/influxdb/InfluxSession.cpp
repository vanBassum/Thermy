#include "InfluxSession.h"
#include "esp_log.h"
#include <cstdio>
#include <cstring>

static const char* TAG = "InfluxSession";

// -----------------------------------------------------------------------------
// Constructor
// -----------------------------------------------------------------------------
InfluxSession::InfluxSession(const char* url,
                             const char* apiKey,
                             const char* measurementName,
                             const DateTime& timestamp,
                             TickType_t timeout)
    : _req(nullptr, HTTP_METHOD_POST, timeout),
      _stream(),
      _writer(_stream),
      _timestamp(timestamp),
      _hasFields(false),
      _open(false)
{
    HttpClient http;
    _req = http.createRequest(url, HTTP_METHOD_POST, timeout);

    _req.setHeader("Content-Type", "text/plain; charset=utf-8");
    _req.setHeader("Transfer-Encoding", "chunked");

    if (apiKey && std::strlen(apiKey) > 0) {
        char authHeader[128];
        std::snprintf(authHeader, sizeof(authHeader), "Token %s", apiKey);
        _req.setHeader("Authorization", authHeader);
    }

    if (!_req.open()) {
        ESP_LOGE(TAG, "Failed to open HTTP connection for Influx write");
        return;
    }

    _stream = _req.createStream();

    // Start first measurement line
    _writer.writeString(measurementName);
    _writer.writeChar(' ');

    _open = true;
}

// -----------------------------------------------------------------------------
// Destructor
// -----------------------------------------------------------------------------
InfluxSession::~InfluxSession()
{
    if (_open) {
        Finish();
    }
}

// -----------------------------------------------------------------------------
// Tags
// -----------------------------------------------------------------------------
InfluxSession& InfluxSession::withTag(const char* key, const char* value)
{
    if (!_open) return *this;
    _writer.writeChar(',');
    _writer.writeFormat("%s=%s", key, value);
    return *this;
}

// -----------------------------------------------------------------------------
// Fields
// -----------------------------------------------------------------------------
InfluxSession& InfluxSession::withField(const char* key, float value)
{
    if (!_open) return *this;
    if (!_hasFields) {
        _writer.writeChar(' ');
        _hasFields = true;
    } else {
        _writer.writeChar(',');
    }
    _writer.writeFormat("%s=", key);
    _writer.writeFloat(value);
    return *this;
}

InfluxSession& InfluxSession::withField(const char* key, double value)
{
    if (!_open) return *this;
    if (!_hasFields) {
        _writer.writeChar(' ');
        _hasFields = true;
    } else {
        _writer.writeChar(',');
    }
    _writer.writeFormat("%s=", key);
    _writer.writeFloat(static_cast<float>(value));
    return *this;
}

InfluxSession& InfluxSession::withField(const char* key, int32_t value)
{
    if (!_open) return *this;
    if (!_hasFields) {
        _writer.writeChar(' ');
        _hasFields = true;
    } else {
        _writer.writeChar(',');
    }
    _writer.writeFormat("%s=%di", key, value);  // int fields use 'i' suffix
    return *this;
}

InfluxSession& InfluxSession::withField(const char* key, bool value)
{
    if (!_open) return *this;
    if (!_hasFields) {
        _writer.writeChar(' ');
        _hasFields = true;
    } else {
        _writer.writeChar(',');
    }
    _writer.writeFormat("%s=%s", key, value ? "true" : "false");
    return *this;
}

// -----------------------------------------------------------------------------
// New measurement
// -----------------------------------------------------------------------------
InfluxSession& InfluxSession::withMeasurement(const char* name, const DateTime& timestamp)
{
    if (!_open) return *this;

    // End previous line with timestamp
    _writer.writeChar(' ');
    _writer.writeInt64(_timestamp.UtcSeconds());
    _writer.writeChar('\n');

    // Start new measurement
    _writer.writeString(name);
    _writer.writeChar(' ');

    _timestamp = timestamp;
    _hasFields = false;
    return *this;
}

// -----------------------------------------------------------------------------
// Finish
// -----------------------------------------------------------------------------
bool InfluxSession::Finish()
{
    if (!_open) return false;

    // End current line with timestamp
    _writer.writeChar(' ');
    _writer.writeInt64(_timestamp.UtcSeconds());
    _writer.writeChar('\n');

    _stream.flush();
    _stream.close();

    // Check server response
    int status = _req.getStatusCode();
    ESP_LOGI(TAG, "Influx write HTTP %d", status);

    _req.close();
    _open = false;

    bool ok = (status == 204);
    if (ok) {
        ESP_LOGI(TAG, "Influx write acknowledged");
    } else {
        ESP_LOGW(TAG, "Influx write failed with HTTP %d", status);
    }

    return ok;
}
