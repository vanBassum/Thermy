#include "HttpServerResponseStream.h"

HttpServerResponseStream::HttpServerResponseStream(httpd_req_t *r)
    : req(r)
{
}

HttpServerResponseStream::~HttpServerResponseStream()
{
    flush();
}

size_t HttpServerResponseStream::write(const void *data, size_t len)
{
    int ret = httpd_resp_send_chunk(req, (const char *)data, len);
    if (ret == ESP_OK)
    {
        return len;
    }
    return 0;
}

size_t HttpServerResponseStream::read(void *buffer, size_t len)
{
    assert(false && "ResponseStream does not support read()");
    return 0; // not supported
}

void HttpServerResponseStream::flush()
{
    // do nothing, just keep interface happy
}

