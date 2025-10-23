#pragma once
#include "Stream.h"
#include "esp_http_server.h"

class HttpServerResponseStream : public Stream
{
public:
    explicit HttpServerResponseStream(httpd_req_t *r);
    ~HttpServerResponseStream();
    size_t write(const void *data, size_t len) override;
    size_t read(void *buffer, size_t len) override;
    void flush() override;
    
private:
    httpd_req_t *req;
};
