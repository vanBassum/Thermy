#include "HttpClient.h"
#include "esp_log.h"
#include "esp_crt_bundle.h"

static const char* TAG = "HttpClient";

static esp_err_t http_evt(esp_http_client_event_t *evt) {
    switch (evt->event_id) {
        case HTTP_EVENT_ON_HEADER:
            if (evt->header_key && evt->header_value) {
                ESP_LOGI("HTTP_EVENT", "%s: %s", evt->header_key, evt->header_value);
            }
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGI("HTTP_EVENT", "Response chunk (%d bytes)", (int)evt->data_len);
            break;
        default:
            break;
    }
    return ESP_OK;
}

bool HttpClient::Init(const char* baseUrl) {
    if(initGuard.IsReady())
        return true;
    _baseUrl = baseUrl;

    esp_http_client_config_t config = {};
    config.url = baseUrl;
    config.timeout_ms = 5000;
    config.disable_auto_redirect = false;
    config.crt_bundle_attach = esp_crt_bundle_attach;
    //config.event_handler = http_evt;


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
