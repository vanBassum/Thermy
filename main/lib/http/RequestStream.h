#pragma once
#include "Stream.h"
#include "esp_http_client.h"
#include "esp_log.h"

// Writes directly to ESP-IDF esp_http_client using POST body streaming
class RequestStream : public Stream {
    esp_http_client_handle_t _client;
    bool _closed = true;

public:
    RequestStream() : _client(nullptr), _closed(true) {}
    explicit RequestStream(esp_http_client_handle_t client)
        : _client(client), _closed(false) {}

    size_t write(const void* data, size_t len) override {
        if (_closed || !_client || len == 0)
            return 0;

        // Send chunk size in hex + CRLF
        char header[10];
        int header_len = snprintf(header, sizeof(header), "%x\r\n", (unsigned int)len);
        esp_http_client_write(_client, header, header_len);

        // Send data
        esp_http_client_write(_client, static_cast<const char*>(data), len);

        // Send trailing CRLF
        esp_http_client_write(_client, "\r\n", 2);
        return len;
    }

    size_t read(void*, size_t) override {
        return 0; // not used for Influx uploads
    }

    void flush() override {
        // HTTP client doesn't need flush
    }

    void close() {
        if (!_closed && _client) {
            // send final zero-length chunk
            esp_http_client_write(_client, "0\r\n\r\n", 5);
            _closed = true;
        }
    }

    bool isOpen() const { return !_closed; }
};
