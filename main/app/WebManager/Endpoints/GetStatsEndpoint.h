#pragma once
#include "HttpServerEndpoint.h"
#include "json/json.h"
#include "ServiceProvider.h"
#include "DataManager.h"
#include "SimpleStats.h"
#include "BufferedStream.h"


class GetStatsEndpoint : public HttpServerEndpoint
{
public:
    explicit GetStatsEndpoint(ServiceProvider& services)
        : services(services)
    {
    }

    esp_err_t handle(httpd_req_t* req) override
    {
        httpd_resp_set_type(req, "application/json");
        HttpServerResponseStream stream(req);
        BufferedStream<128> bufferedStream(stream);

        SimpleStats* stats = services.GetStats();
        size_t statsCount = services.GetStatsCount();
        
        JsonArrayWriter::create(bufferedStream, [&](JsonArrayWriter &array)
        {
            for(int i=0; i<statsCount; i++)
            {
                const SimpleStats& stat = stats[i];
                array.withObject([&](JsonObjectWriter &obj)
                {
                    obj.field("name", stat.Name());
                    obj.field("count", stat.Count());
                    obj.field("min", stat.Min());
                    obj.field("max", stat.Max());
                    obj.field("avg", stat.Avg());
                    obj.field("last", stat.Last());
                    obj.field("stddev", stat.StdDev());

                });
            }
        });
        bufferedStream.flush();
        stream.flush();
        httpd_resp_send_chunk(req, NULL, 0);
        return ESP_OK;
    }

private:
    ServiceProvider& services;
};
