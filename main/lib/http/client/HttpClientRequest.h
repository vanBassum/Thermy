#pragma once
#include "HttpClient.h"
#include "HttpClientStream.h"
#include "esp_http_client.h"
#include "esp_err.h"
#include "esp_log.h"

class HttpClientRequest {
public:
    enum class State {
        Idle,
        Writing,
        Reading,
        Completed,
        Error
    };

    HttpClientRequest(HttpClient& client);
    ~HttpClientRequest();

    void SetMethod(esp_http_client_method_t method);
    void SetPath(const char* path);
    void AddHeader(const char* key, const char* value);
    bool Begin();          // Opens connection for writing
    int Finalize();        // Fetches headers, returns HTTP status
    HttpClientStream& GetStream();

    // Accessor for stream validation
    State GetState() const { return _state; }

private:
    friend class HttpClientStream;  // allow stream to read _state

    esp_http_client_handle_t _handle = nullptr;
    HttpClientStream _stream;
    State _state = State::Idle;
    inline static constexpr const char* TAG = "HttpClientRequest";
};
