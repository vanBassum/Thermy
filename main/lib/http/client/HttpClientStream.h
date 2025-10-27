#pragma once
#include "Stream.h"
#include "esp_http_client.h"
#include "esp_log.h"

class HttpClientRequest;  // forward declaration

class HttpClientStream : public Stream {
public:
    HttpClientStream(esp_http_client_handle_t handle, HttpClientRequest& request);
    size_t write(const void* data, size_t len) override;
    size_t read(void* buffer, size_t len) override;
    void flush() override;

private:
    esp_http_client_handle_t _handle = nullptr;
    HttpClientRequest& _request;
    inline static constexpr const char* TAG = "HttpClientStream";
};
