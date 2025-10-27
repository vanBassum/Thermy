#include "HttpClient.h"
#include "esp_log.h"
#include "esp_crt_bundle.h"

static const char* TAG = "HttpClient";

bool HttpClient::Init(const char* baseUrl) {
    if(initGuard.IsReady())
        return true;
    _baseUrl = baseUrl;

    esp_http_client_config_t config = {};
    config.url = baseUrl;
    config.timeout_ms = 5000;
    config.disable_auto_redirect = false;
    config.crt_bundle_attach = esp_crt_bundle_attach;

    _handle = esp_http_client_init(&config);
    if (!_handle) {
        ESP_LOGE(TAG, "Failed to init HTTP client");
        return false;
    }

    ESP_LOGI(TAG, "HTTP client initialized for base URL: %s", baseUrl);
    initGuard.SetReady();
    return true;
}

HttpClient::~HttpClient() {
    if (_handle) {
        esp_http_client_cleanup(_handle);
        _handle = nullptr;
    }
}
