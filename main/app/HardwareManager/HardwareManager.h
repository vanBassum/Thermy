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

        ESP_LOGI(TAG, "Initializing I2C bus...");

//        i2c_master_bus_config_t bus_cfg = {};
//        bus_cfg.clk_source = I2C_CLK_SRC_DEFAULT;
//        bus_cfg.i2c_port = I2C_PORT;
//        bus_cfg.sda_io_num = SDA_PIN;
//        bus_cfg.scl_io_num = SCL_PIN;
//        bus_cfg.glitch_ignore_cnt = 7;
//        bus_cfg.flags.enable_internal_pullup = true;
//            
//        
//
//        esp_err_t err = i2c_new_master_bus(&bus_cfg, &busHandle);
//        if (err != ESP_OK)
//        {
//            ESP_LOGE(TAG, "Failed to initialize I2C bus: %s", esp_err_to_name(err));
//            return err;
//        }

//        ESP_LOGI(TAG, "I2C bus initialized successfully.");
        initGuard.SetReady();
        return ESP_OK;
    }
    
    
    void Tick(TickContext& ctx){};

    //[[nodiscard]] i2c_master_bus_handle_t GetI2CBus() const
    //{
    //    REQUIRE_READY(initGuard);
    //    return busHandle;
    //}

private:
    InitGuard initGuard;
    //i2c_master_bus_handle_t busHandle = nullptr;
    FatfsDriver fatFsDriver{"/fat", "fat"};
};
