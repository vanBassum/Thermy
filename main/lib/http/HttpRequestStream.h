#pragma once
#include "Stream.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include <cstddef>
#include <cstring>

/// Stream wrapper around esp_http_client.
/// Provides a write/read interface compatible with the generic Stream class.
class HttpRequestStream : public Stream
{
    inline static constexpr const char* TAG = "HttpRequestStream";

public:
    HttpRequestStream() = default;
    ~HttpRequestStream() override = default;

    /// Initialize the stream with an existing HTTP client handle.
    /// Must be called after the client is opened.
    void Init(esp_http_client_handle_t client, bool chunked = true);

    // --- Stream interface ---
    size_t write(const void* data, size_t len) override;
    size_t read(void* buffer, size_t len) override;
    void flush() override;

    /// Closes the stream (writes final chunk if chunked).
    void close();

    bool isOpen() const { return _client != nullptr; }

private:
    esp_http_client_handle_t _client = nullptr;
    bool _chunked = true;
};
