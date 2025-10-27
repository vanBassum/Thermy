#include "InfluxSession.h"
#include <cassert>
#include <cstdio>
#include <cstring>

InfluxSession::InfluxSession(HttpClient& client,
                             const char* endpoint,
                             const char* apiKey,
                             TickType_t timeout)
    : _req(client)
    , _bufferedStream(_req.GetStream())
{
    _req.SetMethod(HTTP_METHOD_POST);
    _req.SetPath(endpoint);

    if (apiKey && *apiKey) {
        char auth[160];
        snprintf(auth, sizeof(auth), "Token %s", apiKey);
        _req.AddHeader("Authorization", auth);
    }
    _req.AddHeader("Content-Type", "text/plain; charset=utf-8");
    _req.AddHeader("Connection", "close");
    _req.AddHeader("Transfer-Encoding", "chunked");

    if (!_req.Begin()) {
        ESP_LOGE(TAG, "Failed to start request");
        return;
    }

    _initGuard.SetReady();
    _phase = WritePhase::None;
}

InfluxSession::~InfluxSession()
{
}


InfluxSession& InfluxSession::withMeasurement(const char* name, const DateTime& timestamp)
{
    REQUIRE_READY(_initGuard);
    assert(name);

    StringWriter writer(_bufferedStream);

    // If this isn't the first measurement, end the previous line
    if (_phase != WritePhase::None) {
        writer.writeChar(' ');
        writer.writeInt64(_timestamp.UtcSeconds());
        writer.writeChar('\n');
    }

    writer.writeString(name);
    _timestamp = timestamp;
    _phase = WritePhase::Measurement;
    return *this;
}

InfluxSession& InfluxSession::withTag(const char* key, const char* value)
{
    REQUIRE_READY(_initGuard);
    assert(key && value);
    assert(_phase == WritePhase::Measurement || _phase == WritePhase::Tags);

    StringWriter writer(_bufferedStream);

    writer.writeChar(',');
    WriteEscaped(key);
    writer.writeChar('=');
    WriteEscaped(value);

    _phase = WritePhase::Tags;
    return *this;
}

InfluxSession& InfluxSession::withField(const char* key, float value)
{
    REQUIRE_READY(_initGuard);
    assert(key);
    assert(_phase == WritePhase::Measurement || _phase == WritePhase::Tags || _phase == WritePhase::Fields);

    StringWriter writer(_bufferedStream);

    if (_phase != WritePhase::Fields)
        writer.writeChar(' ');

    WriteEscaped(key);
    writer.writeChar('=');

    char buf[32];
    int len = std::snprintf(buf, sizeof(buf), "%.6f", value);
    writer.writeString(buf, len);

    _phase = WritePhase::Fields;
    return *this;
}

InfluxSession& InfluxSession::withField(const char* key, int32_t value)
{
    REQUIRE_READY(_initGuard);
    assert(key);
    assert(_phase == WritePhase::Measurement || _phase == WritePhase::Tags || _phase == WritePhase::Fields);

    StringWriter writer(_bufferedStream);

    if (_phase != WritePhase::Fields)
        writer.writeChar(' ');

    WriteEscaped(key);
    writer.writeChar('=');
    writer.writeInt64(value);
    writer.writeChar('i');

    _phase = WritePhase::Fields;
    return *this;
}

InfluxSession& InfluxSession::withField(const char* key, bool value)
{
    REQUIRE_READY(_initGuard);
    assert(key);
    assert(_phase == WritePhase::Measurement || _phase == WritePhase::Tags || _phase == WritePhase::Fields);

    StringWriter writer(_bufferedStream);

    if (_phase != WritePhase::Fields)
        writer.writeChar(' ');

    WriteEscaped(key);
    writer.writeChar('=');
    writer.writeString(value ? "true" : "false");

    _phase = WritePhase::Fields;
    return *this;
}

bool InfluxSession::Finish()
{
    if (!_initGuard.IsReady() || _phase == WritePhase::None)
        return false;

    StringWriter writer(_bufferedStream);
    writer.writeChar(' ');
    writer.writeInt64(_timestamp.UtcSeconds());
    writer.writeChar('\n');
    _bufferedStream.flush();

    int status = _req.Finalize();

    if (status < 0) {
        ESP_LOGE(TAG, "Influx write failed, HTTP %d", status);
        _req.Close(); // <--- Ensure socket closed even on error
        return false;
    }

    // Optionally read body if needed (e.g. error message)
    char buf[128];
    int len = _bufferedStream.read(buf, sizeof(buf) - 1);
    if (len > 0) {
        buf[len] = 0;
        ESP_LOGW(TAG, "Influx response body: %s", buf);
    }

    _req.Close(); // <--- Always close after reading

    if (status < 200 || status >= 300) {
        ESP_LOGE(TAG, "Influx write failed, HTTP %d", status);
        return false;
    }
    return true;
}



void InfluxSession::WriteEscaped(const char* text)
{
    assert(text);
    for (const char* p = text; *p; ++p)
    {
        char c = *p;
        if (c == ' ' || c == ',' || c == '=')
            _bufferedStream.write("\\", 1);
        _bufferedStream.write(&c, 1);
    }
}
