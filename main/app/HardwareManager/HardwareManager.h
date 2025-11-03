#pragma once
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "FatfsDriver.h"
#include "rtos.h"
#include "TickContext.h"
#include "ServiceProvider.h"

class HardwareManager
{
    inline static constexpr const char *TAG = "HardwareManager";
    constexpr static const i2c_port_t I2C_PORT = I2C_NUM_0;
    constexpr static const gpio_num_t SDA_PIN = GPIO_NUM_5;
    constexpr static const gpio_num_t SCL_PIN = GPIO_NUM_6;
    constexpr static const uint32_t I2C_FREQ_HZ = 400000;

public:
    explicit HardwareManager(ServiceProvider& services){};

    esp_err_t Init()
    {
        if(initGuard.IsReady())
            return ESP_OK;

        fatFsDriver.Init();

        initGuard.SetReady();
        return ESP_OK;
    }
    
    
    void Tick(TickContext& ctx){};
private:
    InitGuard initGuard;
    FatfsDriver fatFsDriver{"/fat", "fat"};
};
