#include "HttpClientRequestStream.h"
#include "esp_err.h"
#include <cstdio>
#include <cassert>

void HttpClientRequestStream::Init(esp_http_client_handle_t client, bool chunked)
{
    _client = client;
    _chunked = chunked;
}

size_t HttpClientRequestStream::write(const void* data, size_t len)
{
    assert(_client && "Client not initialized");
    assert(data && len > 0);

    // For debuffing
    printf("%.*s", (int)len, (const char*)data);

    if (_chunked) {
        char header[10];
        int header_len = snprintf(header, sizeof(header), "%x\r\n", (unsigned int)len);
        esp_http_client_write(_client, header, header_len);
        esp_http_client_write(_client, static_cast<const char*>(data), len);
        esp_http_client_write(_client, "\r\n", 2);
    } else {
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

void HttpClientRequestStream::flush() {
    // No-op: writes are immediate, included for interface symmetry
}

