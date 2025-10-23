#pragma once
#include "HttpServerEndpoint.h"
#include "json.h"


class TestEndpoint : public HttpServerEndpoint
{
public:
    explicit TestEndpoint() = default;

    esp_err_t handle(httpd_req_t* req) override
    {
        httpd_resp_set_type(req, "application/json");
        HttpServerResponseStream stream(req);
        JsonObjectWriter::create(stream, [&](JsonObjectWriter &json)
                                 { json.field("status", "ok"); });
        stream.flush();
        return ESP_OK;
    }

private:
};
