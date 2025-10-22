#include <stdio.h>
#include "esp_log.h"
#include "AppContext.h"
#include "NvsStorage.h"
#include "InfluxClient.h"
#include "core_utils.h"
#include "SSD1306.h"

constexpr const char *TAG = "Main";

AppContext appContext;

extern "C" void app_main(void)
{
    esp_log_level_set("esp-x509-crt-bundle", ESP_LOG_WARN);

    // ---- I2C setup ----
    const i2c_port_t i2c_port = I2C_NUM_0;
    const gpio_num_t sda_gpio = GPIO_NUM_5;
    const gpio_num_t scl_gpio = GPIO_NUM_6;
    const uint32_t i2c_freq = 400000;

    i2c_config_t conf = {};
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = sda_gpio;
    conf.scl_io_num = scl_gpio;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = i2c_freq;

    ESP_ERROR_CHECK(i2c_param_config(i2c_port, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(i2c_port, conf.mode, 0, 0, 0));

    // ---- OLED init ----
    const uint8_t oled_width = 128;
    const uint8_t oled_height = 64;
    const uint8_t oled_addr = 0x3C;
    const bool external_vcc = false;

    ESP_LOGI(TAG, "Initializing SSD1306 at %dx%d", oled_width, oled_height);
    SSD1306_I2C display(oled_width, oled_height, i2c_port, oled_addr, external_vcc);

    display.initDisplay();
    display.contrast(0xFF);
    display.fill(0);
    display.show();

    // ---- Demo loop ----
    while (true) {
        ESP_LOGI(TAG, "All black");
        display.fill(0);
        display.show();
        vTaskDelay(pdMS_TO_TICKS(1000));

        ESP_LOGI(TAG, "All white");
        display.fill(1);
        display.show();
        vTaskDelay(pdMS_TO_TICKS(1000));

        ESP_LOGI(TAG, "Diagonal");
        display.fill(0);
        for (int i = 0; i < 64; ++i)
            display.drawPixel(i, i, true);
        display.show();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }



    // Initialize NVS
    ESP_LOGI(TAG, "Initializing NVS...");
    NvsStorage::InitNvsPartition("nvs");
    NvsStorage::PrintStats("nvs");

    // Initialize application services
    appContext.GetSettingsManager().Init();
    appContext.GetWifiManager().Init();
    appContext.GetTimeManager().Init();
    appContext.GetSensorManager().Init();
    appContext.GetInfluxManager().Init();

    // --------------------------------------------------------
    // Main loop
    // --------------------------------------------------------
    ESP_LOGI(TAG, "Entering main loop...");
    while (true)
    {
        appContext.GetWifiManager().Loop();

        if (appContext.GetTimeManager().IsTimeValid())
        {
            float temp;
            onewire_device_address_t addr;
            if (appContext.GetSensorManager().GetTemperature(0, temp, addr))
            {
                StringConverter converter;
                char addrStr[17];
                converter.BlobToString(addrStr, sizeof(addrStr), &addr, sizeof(addr));

                DateTime now = DateTime::Now();
                appContext.GetInfluxManager().BeginWrite("temperature", now, pdMS_TO_TICKS(2000)).withTag("sensor", "ds18b20").withTag("address", addrStr).withField("value", temp).Finish();
            }
            else
            {
                ESP_LOGW(TAG, "No temperature sensor available");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
