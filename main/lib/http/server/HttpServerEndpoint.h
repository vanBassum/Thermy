#pragma once
#include "esp_err.h"
#include "esp_http_server.h"

class HttpServerEndpoint {
public:
    HttpServerEndpoint() = default;
    virtual ~HttpServerEndpoint() = default;

    HttpServerEndpoint(const HttpServerEndpoint&) = delete;
    HttpServerEndpoint& operator=(const HttpServerEndpoint&) = delete;
    HttpServerEndpoint(HttpServerEndpoint&&) = delete;
    HttpServerEndpoint& operator=(HttpServerEndpoint&&) = delete;

    virtual esp_err_t handle(httpd_req_t* req) = 0;
};
