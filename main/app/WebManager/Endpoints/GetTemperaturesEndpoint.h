#pragma once
#include "HttpServerEndpoint.h"
#include "json.h"
#include "ServiceProvider.h"
#include "DataManager.h"


class GetTemperaturesEndpoint : public HttpServerEndpoint
{
public:
    explicit GetTemperaturesEndpoint(ServiceProvider& services)
        : dataManager(services.GetDataManager())
    {
    }

    esp_err_t handle(httpd_req_t* req) override
    {
        httpd_resp_set_type(req, "application/json");
        HttpServerResponseStream stream(req);
        BufferedStream<128> bufferedStream(stream);
        
        JsonArrayWriter::create(bufferedStream, [&](JsonArrayWriter &array)
        {
            dataManager.ForEach([&](DataEntry &entry){

                array.withObject([&](JsonObjectWriter &obj)
                {
                    auto valuePair = entry.FindPair(DataKey::Value);
                    auto addressPair = entry.FindPair(DataKey::Address);

                    if (!valuePair || !addressPair)
                        return;
                                            
                    char addressStr[17] = {};
                    snprintf(addressStr, sizeof(addressStr), "%016llX", addressPair->value.asUint64);

                    char timestampStr[25] = {};
                    entry.timestamp.ToStringUtc(timestampStr, sizeof(timestampStr), DateTime::FormatIso8601);

                    obj.field("address", addressStr);
                    obj.field("timestamp", timestampStr);
                    obj.field("temperature", valuePair->value.asFloat);
                });

            });
        });
        bufferedStream.flush();
        stream.flush();
        httpd_resp_send_chunk(req, NULL, 0);
        return ESP_OK;
    }

private:
    DataManager& dataManager;
};
