#pragma once
#include "esp_http_server.h"
#include <string>
#include <string_view>

class ServerRequest {
public:
    explicit ServerRequest(httpd_req_t* req) : _req(req) {}

    httpd_method_t Method() const { return _req->method; }
    const char* Uri() const { return _req->uri ? _req->uri : ""; }

    std::string GetHeader(const char* key) const {
        size_t len = httpd_req_get_hdr_value_len(_req, key);
        if (len == 0) return {};
        std::string val; val.resize(len + 1);
        if (httpd_req_get_hdr_value_str(_req, key, val.data(), val.size()) == ESP_OK) {
            val.resize(strlen(val.c_str()));
            return val;
        }
        return {};
    }

    // Read request body in chunks; returns bytes read (0 = no more, <0 error)
    int ReadBody(char* buf, size_t len) {
        int r = httpd_req_recv(_req, buf, len);
        return r; // ESP-IDF returns <=0 on error/closed; you decide policy at call site
    }

    size_t ContentLength() const { return _req->content_len; }

    httpd_req_t* Raw() const { return _req; }

private:
    httpd_req_t* _req;
};