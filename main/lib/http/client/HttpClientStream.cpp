#include "HttpClientStream.h"
#include "HttpClientRequest.h"

HttpClientStream::HttpClientStream(esp_http_client_handle_t handle, HttpClientRequest& request)
    : _handle(handle), _request(request) 
{
}

size_t HttpClientStream::write(const void *data, size_t len)
{
    if (!_handle) {
        ESP_LOGE(TAG, "Invalid stream handle");
        return 0;
    }

    if (_request.GetState() != HttpClientRequest::State::Writing) {
        ESP_LOGE(TAG, "Write called in invalid state (%d)", (int)_request.GetState());
        return 0;
    }

    int written = esp_http_client_write(_handle, (const char*)data, len);
    if (written < 0) {
        ESP_LOGE(TAG, "esp_http_client_write failed");
        _request._state = HttpClientRequest::State::Error;
        return 0;
    }

    return (size_t)written;
}

size_t HttpClientStream::read(void* buffer, size_t len) {
    if (!_handle) {
        ESP_LOGE(TAG, "Invalid stream handle");
        return 0;
    }

    if (_request.GetState() != HttpClientRequest::State::Reading) {
        ESP_LOGE(TAG, "Read called in invalid state (%d)", (int)_request.GetState());
        return 0;
    }

    int readLen = esp_http_client_read(_handle, (char*)buffer, len);
    if (readLen < 0) {
        ESP_LOGE(TAG, "esp_http_client_read failed");
        _request._state = HttpClientRequest::State::Error;
        return 0;
    }

    return (size_t)readLen;
}

void HttpClientStream::flush() {
    // Optional — can be used later for chunked transfers
}
