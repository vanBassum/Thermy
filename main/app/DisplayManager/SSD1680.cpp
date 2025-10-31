#include "SSD1680.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


bool SSD1680::IsBusy() const
{
    int lvl = gpio_get_level(_pinBusy);
    return BUSY_ACTIVE_LOW ? (lvl == 0) : (lvl == 1);
}

bool SSD1680::WaitBusy(uint32_t timeoutMs) const
{
    uint32_t waited = 0;
    while (IsBusy() && waited < timeoutMs) {
        vTaskDelay(pdMS_TO_TICKS(1));
        ++waited;
    }
    return !IsBusy();
}

void SSD1680::WriteCmd(uint8_t cmd) const
{
    gpio_set_level(_pinDc, 0);
    spi_transaction_t t = {};
    t.length = 8;
    t.tx_buffer = &cmd;
    ESP_ERROR_CHECK(spi_device_transmit(_spi, &t));
}

void SSD1680::WriteData(const uint8_t* data, size_t len) const
{
    gpio_set_level(_pinDc, 1);
    spi_transaction_t t = {};
    t.length = len * 8;
    t.tx_buffer = data;
    ESP_ERROR_CHECK(spi_device_transmit(_spi, &t));
}

esp_err_t SSD1680::Init(spi_device_handle_t spi,
                        gpio_num_t pinDc,
                        gpio_num_t pinRst,
                        gpio_num_t pinBusy)
{
    _spi = spi;
    _pinDc = pinDc;
    _pinRst = pinRst;
    _pinBusy = pinBusy;

    // --- Reset ---
    gpio_set_level(_pinRst, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(_pinRst, 1);
    vTaskDelay(pdMS_TO_TICKS(10));

    // --- Software reset ---
    WriteCmd(0x12);
    WaitBusy();

    // --- Driver output control ---
    uint8_t driverCtrl[3] = {
        (uint8_t)((HEIGHT - 1) & 0xFF),
        (uint8_t)(((HEIGHT - 1) >> 8) & 0xFF),
        0x00
    };
    WriteCmd(0x01);
    WriteData(driverCtrl, 3);

    // --- Booster soft start ---
    uint8_t boosterSoftStart[3] = { 0xD7, 0xD6, 0x9D };
    WriteCmd(0x0C);
    WriteData(boosterSoftStart, 3);

    // --- VCOM voltage ---
    uint8_t vcom = 0xA8;
    WriteCmd(0x2C);
    WriteData(&vcom, 1);

    // --- Dummy line and gate width ---
    uint8_t dummyLine = 0x1A;
    uint8_t gateWidth = 0x08;
    WriteCmd(0x3A);
    WriteData(&dummyLine, 1);
    WriteCmd(0x3B);
    WriteData(&gateWidth, 1);

    // --- Data entry mode ---
    uint8_t dataEntry = 0x03;
    WriteCmd(0x11);
    WriteData(&dataEntry, 1);

    // --- Set RAM X/Y range ---
    uint8_t xRange[2] = { 0x01, (uint8_t)(((WIDTH - 1) >> 3) + 1) };
    WriteCmd(0x44);
    WriteData(xRange, 2);

    uint8_t yRange[4] = {
        0x00, 0x00,
        (uint8_t)((HEIGHT - 1) & 0xFF),
        (uint8_t)(((HEIGHT - 1) >> 8) & 0xFF)
    };
    WriteCmd(0x45);
    WriteData(yRange, 4);

    // --- Set RAM counters ---
    uint8_t xStart = 0x01;
    WriteCmd(0x4E);
    WriteData(&xStart, 1);
    WriteCmd(0x4F);
    uint8_t yz[2] = { 0x00, 0x00 };
    WriteData(yz, 2);
    WaitBusy();

    ESP_LOGI(TAG, "Initialized SSD1680 display");
    return ESP_OK;
}

void SSD1680::WriteFrame(const uint8_t* frameData)
{
    WriteCmd(0x24);
    WriteData(frameData, FRAME_BYTES);
}

void SSD1680::Update()
{
    uint8_t displayUpdate = 0xF7; // full refresh
    WriteCmd(0x22);
    WriteData(&displayUpdate, 1);
    WriteCmd(0x20); // Master activate
    WaitBusy(10000);
}

void SSD1680::Sleep()
{
    WriteCmd(0x10);
    uint8_t sleep = 0x01;
    WriteData(&sleep, 1);
}

void SSD1680::WakeUp()
{
    gpio_set_level(_pinRst, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(_pinRst, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
}
