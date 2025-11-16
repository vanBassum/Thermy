#pragma once
#include "HttpServerEndpoint.h"
#include "json/json.h"
#include "ServiceProvider.h"
#include "BufferedStream.h"
#include "SensorManager.h"


class GetTemperaturesEndpoint : public HttpServerEndpoint
{
public:
    explicit GetTemperaturesEndpoint(ServiceProvider& services)
        : sensorManager(services.GetSensorManager())
    {
    }

    esp_err_t handle(httpd_req_t* req) override
    {
        httpd_resp_set_type(req, "application/json");
        HttpServerResponseStream stream(req);
        BufferedStream<128> bufferedStream(stream);
        
        JsonArrayWriter::create(bufferedStream, [&](JsonArrayWriter &array)
        {
            int count = sensorManager.GetSensorCount();

            for(int i=0; i<count; i++)
            {
                float temperature = sensorManager.GetTemperature(i);
                uint64_t address = sensorManager.GetAddress(i);

                array.withObject([&](JsonObjectWriter &obj)
                {
                    obj.field("address", address);
                    obj.field("temperature", temperature);
                });
            }
        });
        bufferedStream.flush();
        stream.flush();
        httpd_resp_send_chunk(req, NULL, 0);
        return ESP_OK;
    }

private:
    SensorManager& sensorManager;
};
