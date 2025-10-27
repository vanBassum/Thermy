#include "InfluxSession.h"
#include <cassert>

enum class WritePhase
{
    None,
    Measurement,
    Tags,
    Fields
};

InfluxSession::~InfluxSession()
{
}

void InfluxSession::Init(const char *url, const char *apiKey, TickType_t timeout)
{
    if (_initGuard.IsReady())
        return;

    _req.Init(url, HTTP_METHOD_POST, timeout);
    

    if (apiKey && *apiKey)
    {
        char authHeader[160];
        snprintf(authHeader, sizeof(authHeader), "Token %s", apiKey);
        _req.SetHeader("Authorization", authHeader);
    }
    _req.SetHeader("Content-Type", "text/plain; charset=utf-8");

    if (!_req.Open())
    {
        ESP_LOGE(TAG, "Failed to open HTTP request");
        return;
    }

    ESP_LOGI(TAG, "InfluxSession initialized for URL: %s", url);

    _initGuard.SetReady();
    _phase = WritePhase::None;
}

InfluxSession &InfluxSession::withMeasurement(const char *name, const DateTime &timestamp)
{
    REQUIRE_READY(_initGuard);
    assert(name);
    
    ESP_LOGI(TAG, "Adding measurement: %s", name);

    auto &stream = _req.GetStream();
    StringWriter writer(stream);

    // If this isn't the first measurement, end the previous line
    if (_phase != WritePhase::None)
    {
        writer.writeChar(' ');
        writer.writeInt64(_timestamp.UtcSeconds());
        writer.writeChar('\n');
    }

    writer.writeString(name);

    _timestamp = timestamp;
    _phase = WritePhase::Measurement;
    return *this;
}

InfluxSession &InfluxSession::withTag(const char *key, const char *value)
{
    REQUIRE_READY(_initGuard);
    assert(key && value);
    assert(_phase == WritePhase::Measurement || _phase == WritePhase::Tags);

    auto &stream = _req.GetStream();
    StringWriter writer(stream);

    writer.writeChar(',');
    WriteEscaped(key);
    writer.writeChar('=');
    WriteEscaped(value);

    _phase = WritePhase::Tags;
    return *this;
}

InfluxSession &InfluxSession::withField(const char *key, float value)
{
    REQUIRE_READY(_initGuard);
    assert(key);
    assert(_phase == WritePhase::Measurement || _phase == WritePhase::Tags || _phase == WritePhase::Fields);

    auto &stream = _req.GetStream();
    StringWriter writer(stream);

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

InfluxSession &InfluxSession::withField(const char *key, int32_t value)
{
    REQUIRE_READY(_initGuard);
    assert(key);
    assert(_phase == WritePhase::Measurement || _phase == WritePhase::Tags || _phase == WritePhase::Fields);

    auto &stream = _req.GetStream();
    StringWriter writer(stream);

    if (_phase != WritePhase::Fields)
        writer.writeChar(' ');

    WriteEscaped(key);
    writer.writeChar('=');
    writer.writeInt64(value);
    writer.writeChar('i');

    _phase = WritePhase::Fields;
    return *this;
}

InfluxSession &InfluxSession::withField(const char *key, bool value)
{
    REQUIRE_READY(_initGuard);
    assert(key);
    assert(_phase == WritePhase::Measurement || _phase == WritePhase::Tags || _phase == WritePhase::Fields);

    auto &stream = _req.GetStream();
    StringWriter writer(stream);

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

    ESP_LOGI(TAG, "Finalizing Influx write session.");

    auto &stream = _req.GetStream();
    StringWriter writer(stream);
    writer.writeChar(' ');
    writer.writeInt64(_timestamp.UtcSeconds());
    writer.writeChar('\n');

    int status = _req.Perform();
    _req.Close();
    _initGuard.SetNotReady();
    _phase = WritePhase::None;

    if (status < 200 || status >= 300) {
        ESP_LOGE(TAG, "Influx write failed, HTTP %d", status);
        return false;
    }
    return true;
}


void InfluxSession::WriteEscaped(const char *text)
{
    assert(text);

    auto &stream = _req.GetStream();
    for (const char *p = text; *p; ++p)
    {
        char c = *p;
        if (c == ' ' || c == ',' || c == '=')
            stream.write("\\", 1);
        stream.write(&c, 1);
    }
}
