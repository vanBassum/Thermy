#include "InfluxSession.h"

void InfluxSession::Init(const char* url,
                         const char* apiKey,
                         const char* measurementName,
                         const DateTime& timestamp,
                         TickType_t timeout)
{
    if (_initGuard.IsReady())
        return;

    _timestamp = timestamp;
    _req.Init(url, HTTP_METHOD_POST, timeout);

    if (apiKey && *apiKey) {
        char authHeader[160];
        snprintf(authHeader, sizeof(authHeader), "Token %s", apiKey);
        _req.SetHeader("Authorization", authHeader);
    }
    _req.SetHeader("Content-Type", "text/plain; charset=utf-8");

    if (!_req.Open()) {
        ESP_LOGE(TAG, "Failed to open HTTP request");
        return;
    }

    // Start writing the line protocol
    auto& stream = _req.Stream();
    StringWriter writer(stream);
    writer.writeString(measurementName);

    _initGuard.SetReady();
}

InfluxSession::~InfluxSession()
{
    if (_initGuard.IsReady())
    {
        Finish();
        _req.Close();
    }
}

InfluxSession& InfluxSession::withTag(const char* key, const char* value)
{
    if (!_initGuard.IsReady())
        return *this;

    auto& stream = _req.Stream();
    StringWriter writer(stream);

    writer.writeChar(',');
    WriteEscaped(key);
    writer.writeChar('=');
    WriteEscaped(value);
    return *this;
}

InfluxSession& InfluxSession::withField(const char* key, float value)
{
    if (!_initGuard.IsReady())
        return *this;

    auto& stream = _req.Stream();
    StringWriter writer(stream);

    writer.writeChar(' ');
    WriteEscaped(key);
    writer.writeChar('=');
    writer.writeFloat(value, 6);
    return *this;
}

InfluxSession& InfluxSession::withField(const char* key, int32_t value)
{
    if (!_initGuard.IsReady())
        return *this;

    auto& stream = _req.Stream();
    StringWriter writer(stream);

    writer.writeChar(' ');
    WriteEscaped(key);
    writer.writeChar('=');
    writer.writeInt64(value);
    writer.writeChar('i');
    return *this;
}

InfluxSession& InfluxSession::withField(const char* key, bool value)
{
    if (!_initGuard.IsReady())
        return *this;

    auto& stream = _req.Stream();
    StringWriter writer(stream);

    writer.writeChar(' ');
    WriteEscaped(key);
    writer.writeChar('=');
    writer.writeString(value ? "true" : "false");
    return *this;
}

InfluxSession& InfluxSession::withMeasurement(const char* name, const DateTime& timestamp)
{
    if (!_initGuard.IsReady())
        return *this;

    auto& stream = _req.Stream();
    StringWriter writer(stream);

    writer.writeChar('\n');
    writer.writeString(name);

    _timestamp = timestamp;
    return *this;
}

bool InfluxSession::Finish()
{
    if (!_initGuard.IsReady())
        return false;

    auto& stream = _req.Stream();
    StringWriter writer(stream);

    // Append timestamp (in seconds)
    writer.writeChar(' ');
    writer.writeInt64(_timestamp.UtcSeconds());
    writer.writeChar('\n');

    // send final zero-length chunk (HttpRequestStream::close handles this)
    stream.close();

    // Ensure the request is actually performed
    esp_http_client_handle_t h = _req.GetClientHandle();   // add this accessor
    esp_err_t perr = (h ? esp_http_client_perform(h) : ESP_FAIL);
    if (perr != ESP_OK) {
        ESP_LOGE(TAG, "HTTP perform error: %s", esp_err_to_name(perr));
        // Don’t attempt to read if perform failed; status will be -1 or undefined
    }

    int status = _req.GetStatusCode();
    ESP_LOGI(TAG, "Influx write finished (timestamp=%lld, status=%d)",
             static_cast<long long>(_timestamp.UtcSeconds()), status);

    // If we got a response, try to read body for diagnostics (optional)
    if (perr == ESP_OK && h && status >= 400) {
        char buf[256];
        int n = esp_http_client_read(h, buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';
            ESP_LOGE(TAG, "InfluxDB error response: %s", buf);
        } else {
            ESP_LOGE(TAG, "InfluxDB returned status %d, but no response body", status);
        }
    }

    return (status >= 200 && status < 300);
}

void InfluxSession::WriteEscaped(const char* text)
{
    auto& stream = _req.Stream();
    for (const char* p = text; *p; ++p)
    {
        if (*p == ' ' || *p == ',' || *p == '=')
            stream.write("\\", 1);
        stream.write(p, 1);
    }
}
