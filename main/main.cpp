#include <stdio.h>
#include "esp_log.h"
#include "AppContext.h"
#include "NvsStorage.h"
#include "driver/i2c.h"

constexpr const char *TAG = "Main";


#define I2C_MASTER_NUM      I2C_NUM_1
#define I2C_MASTER_SCL_IO   4
#define I2C_MASTER_SDA_IO   5
#define I2C_MASTER_FREQ_HZ  400000
#define OLED_ADDR           0x3C

// Initialize the I2C bus
void i2c_master_init() {
    i2c_config_t conf = {};
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_MASTER_SDA_IO;
    conf.scl_io_num = I2C_MASTER_SCL_IO;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;

    ESP_ERROR_CHECK(i2c_param_config(I2C_MASTER_NUM, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0));

    printf("Scanning I2C bus...\n");

    uint8_t dummy = 0;
    for (uint8_t addr = 1; addr < 127; addr++) {
        esp_err_t res = i2c_master_write_to_device(I2C_MASTER_NUM, addr, &dummy, 1, pdMS_TO_TICKS(10));
        if (res == ESP_OK) {
            printf("✅ Found device at 0x%02X\n", addr);
        }
    }

    printf("Scan complete.\n");
}

static esp_err_t oled_cmd(uint8_t cmd) {
    uint8_t buf[2] = {0x00, cmd};
    return i2c_master_write_to_device(I2C_MASTER_NUM, OLED_ADDR, buf, 2, pdMS_TO_TICKS(100));
}

static esp_err_t oled_data(uint8_t data) {
    uint8_t buf[2] = {0x40, data};
    return i2c_master_write_to_device(I2C_MASTER_NUM, OLED_ADDR, buf, 2, pdMS_TO_TICKS(100));
}


void Test() {
    ESP_LOGI(TAG, "Starting SH1106 test...");

    // Quick presence check
    uint8_t dummy = 0;
    if (i2c_master_write_to_device(I2C_MASTER_NUM, OLED_ADDR, &dummy, 1, pdMS_TO_TICKS(10)) != ESP_OK) {
        ESP_LOGE(TAG, "No OLED found at 0x%02X", OLED_ADDR);
        return;
    }

    vTaskDelay(pdMS_TO_TICKS(100));

    // === SH1106 initialization ===
    oled_cmd(0xAE); // Display OFF
    oled_cmd(0xD5); oled_cmd(0x80); // Set clock divide ratio
    oled_cmd(0xA8); oled_cmd(0x3F); // Multiplex ratio
    oled_cmd(0xD3); oled_cmd(0x00); // Display offset
    oled_cmd(0x40);                 // Start line = 0
    oled_cmd(0xAD); oled_cmd(0x8B); // DC-DC ON
    oled_cmd(0xA1);                 // Segment remap
    oled_cmd(0xC8);                 // COM scan direction
    oled_cmd(0xDA); oled_cmd(0x12); // COM pins config
    oled_cmd(0x81); oled_cmd(0x80); // Contrast
    oled_cmd(0xD9); oled_cmd(0x22); // Pre-charge
    oled_cmd(0xDB); oled_cmd(0x35); // VCOM detect
    oled_cmd(0xA6);                 // Normal display
    oled_cmd(0xAF);                 // Display ON

    ESP_LOGI(TAG, "Initialization complete.");

    // === Fill pattern ===
    for (int page = 0; page < 8; page++) {
        oled_cmd(0xB0 + page);           // Page start
        oled_cmd(0x02);                  // Lower column address (SH1106 offset)
        oled_cmd(0x10);                  // Higher column address
        for (int col = 0; col < 128; col++) {
            oled_data((page % 2) ? 0xFF : 0x00);
        }
    }

    ESP_LOGI(TAG, "Pattern written. You should now see alternating horizontal bars.");

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}


















AppContext appContext;

extern "C" void app_main(void)
{
    
    ESP_LOGI(TAG, "Initializing I2C...");
    i2c_master_init();

    Test();

    vTaskDelay(pdMS_TO_TICKS(10000));


    // Initialize NVS
    ESP_LOGI(TAG, "Initializing NVS...");
    NvsStorage::InitNvsPartition("nvs");
    NvsStorage::PrintStats("nvs");

    // Initialize settings
    ESP_LOGI(TAG, "Initializing SettingsManager...");
    appContext.GetSettingsManager().Init();

    appContext.GetSettingsManager().Access([](RootSettings &settings) {
        ESP_LOGI(TAG, "Current settings:");
        appContext.GetSettingsManager().Print(settings);
    });

    // Initialize Wi-Fi (auto-connect)
    ESP_LOGI(TAG, "Initializing WifiManager...");
    appContext.GetWifiManager().Init();


    // Initialize Display
    ESP_LOGI(TAG, "Initializing DisplayManager...");
    appContext.GetDisplayManager().Init();

    // --------------------------------------------------------
    // Main loop
    // --------------------------------------------------------
    ESP_LOGI(TAG, "Entering main loop...");
    while (true)
    {
        appContext.GetWifiManager().Loop();
        appContext.GetDisplayManager().Loop();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
