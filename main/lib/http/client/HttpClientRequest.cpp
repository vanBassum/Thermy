#include "HttpClientRequest.h"
#include <cstdio>

HttpClientRequest::HttpClientRequest(HttpClient& client)
    : _handle(client.GetHandle()),
      _stream(_handle, this)  // <-- Pass this to stream
{
    if (!_handle) {
        ESP_LOGE(TAG, "Invalid HttpClient handle — did you call Init()?");
        _state = State::Error;
        return;
    }
    _state = State::Idle;
}


void HttpClientRequest::SetMethod(esp_http_client_method_t method)
{
    if (_state != State::Idle) {
        ESP_LOGE(TAG, "SetMethod() called in invalid state (%d)", (int)_state);
        return;
    }
    esp_http_client_set_method(_handle, method);
}

void HttpClientRequest::SetPath(const char * path)
{
    if (_state != State::Idle) {
        ESP_LOGE(TAG, "SetPath() called in invalid state (%d)", (int)_state);
        return;
    }
    esp_http_client_set_url(_handle, path);
}

void HttpClientRequest::AddHeader(const char * key, const char * value)
{
    if (_state != State::Idle) {
        ESP_LOGE(TAG, "AddHeader() called in invalid state (%d)", (int)_state);
        return;
    }
    esp_http_client_set_header(_handle, key, value);
}

bool HttpClientRequest::Begin() {
    if (_state != State::Idle) {
        ESP_LOGE(TAG, "Begin() called in invalid state (%d)", (int)_state);
        return false;
    }

    esp_err_t err = esp_http_client_open(_handle, -1);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open connection: %s", esp_err_to_name(err));
        _state = State::Error;
        return false;
    }

    _state = State::Writing;
    return true;
}

int HttpClientRequest::Finalize() {
    if (_state != State::Writing) {
        ESP_LOGE(TAG, "Finalize() called in invalid state (%d)", (int)_state);
        return -1;
    }

    int64_t contentLength = esp_http_client_fetch_headers(_handle);
    if (contentLength < 0) {
        ESP_LOGE(TAG, "Fetch headers failed");
        _state = State::Error;
        return -1;
    }

    _state = State::Reading;
    return esp_http_client_get_status_code(_handle);
}

HttpClientStream & HttpClientRequest::GetStream()
{
    return _stream;
}
