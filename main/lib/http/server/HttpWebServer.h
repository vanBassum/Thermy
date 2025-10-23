#pragma once
#include "esp_err.h"
#include "esp_http_server.h"
#include "HttpServerEndpoint.h"

class HttpWebServer
{
public:
    HttpWebServer();
    ~HttpWebServer();

    HttpWebServer(const HttpWebServer &) = delete;
    HttpWebServer &operator=(const HttpWebServer &) = delete;
    HttpWebServer(HttpWebServer &&) = delete;
    HttpWebServer &operator=(HttpWebServer &&) = delete;

    void start();
    void stop();
    void EnableCors();
    void registerHandler(const char *uri, httpd_method_t method, HttpServerEndpoint &handler);

private:
    httpd_handle_t server = nullptr;

    static void set_cors_headers(httpd_req_t *req);
    static esp_err_t trampoline(httpd_req_t *req);
    static esp_err_t api_options_handler(httpd_req_t *req);
};
