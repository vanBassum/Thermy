#include "HttpClientRequestStream.h"
#include "esp_err.h"
#include <cstdio>

void HttpClientRequestStream::Init(esp_http_client_handle_t client, bool chunked)
{
    _client = client;
    _chunked = chunked;
}

size_t HttpClientRequestStream::write(const void* data, size_t len)
{
    if (!_client || !data || len == 0)
        return 0;

    if (_chunked)
    {
        // --- Send chunk header in hex + CRLF
        char header[10];
        int header_len = snprintf(header, sizeof(header), "%x\r\n", (unsigned int)len);
        esp_http_client_write(_client, header, header_len);

        
        printf("%.*s", (int)len, (const char*)data);

        // --- Send actual data
        esp_http_client_write(_client, static_cast<const char*>(data), len);

        // --- Send CRLF after data
        esp_http_client_write(_client, "\r\n", 2);
    }
    else
    {
        // Non-chunked direct write
        esp_http_client_write(_client, static_cast<const char*>(data), len);
    }

    return len;
}

size_t HttpClientRequestStream::read(void* buffer, size_t len)
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

void HttpClientRequestStream::flush()
{
    // esp_http_client_write() is synchronous, no explicit flush needed.
}
