#include "HttpRequestStream.h"
#include "esp_err.h"

void HttpRequestStream::Init(esp_http_client_handle_t client, bool chunked)
{
    _client = client;
    _chunked = chunked;
}

size_t HttpRequestStream::write(const void* data, size_t len)
{
    if (!_client || !data || len == 0)
        return 0;

    int written = esp_http_client_write(_client, static_cast<const char*>(data), len);
    if (written < 0)
    {
        ESP_LOGE(TAG, "Write failed");
        return 0;
    }
    return static_cast<size_t>(written);
}

size_t HttpRequestStream::read(void* buffer, size_t len)
{
    if (!_client || !buffer || len == 0)
        return 0;

    int received = esp_http_client_read(_client, static_cast<char*>(buffer), len);
    if (received < 0)
    {
        ESP_LOGE(TAG, "Read failed");
        return 0;
    }
    return static_cast<size_t>(received);
}

void HttpRequestStream::flush()
{
    // esp_http_client_write is synchronous, so no buffering to flush.
}

void HttpRequestStream::close()
{
    if (!_client)
        return;

    if (_chunked)
    {
        // Write end-of-chunk marker (zero-length chunk)
        esp_http_client_write(_client, "0\r\n\r\n", 5);
    }

    _client = nullptr;
}
