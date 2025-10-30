#include "SSD1680.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#define TAG "SSD1680"
#define SSD1680_DUMMY_BYTES 2

// --- helpers
static inline void delay_ms(int ms) { vTaskDelay(pdMS_TO_TICKS(ms)); }

static inline void cs_low(SSD1680_HandleTypeDef *h)  { if (h->cs_pin>=0) gpio_set_level(h->cs_pin, 0); }
static inline void cs_high(SSD1680_HandleTypeDef *h) { if (h->cs_pin>=0) gpio_set_level(h->cs_pin, 1); }

static esp_err_t spi_tx(SSD1680_HandleTypeDef *h, const uint8_t *data, size_t len)
{
  spi_transaction_t t = {0};
  t.length = len * 8;
  t.tx_buffer = data;
  return spi_device_transmit(h->spi, &t);
}

static esp_err_t spi_rx(SSD1680_HandleTypeDef *h, uint8_t *data, size_t len)
{
  spi_transaction_t t = {0};
  t.length   = len * 8;
  t.rxlength = len * 8;
  t.rx_buffer = data;
  // provide dummy TX buffer (full-duplex)
  static uint8_t dummy = 0x00;
  t.tx_buffer = &dummy;
  return spi_device_transmit(h->spi, &t);
}

// --- low-level command helpers (ESP-IDF port of your HAL code)
static esp_err_t SSD1680_Send(SSD1680_HandleTypeDef *hepd, const uint8_t command, const uint8_t *pData, size_t size)
{
#ifdef CONFIG_SSD1680_DEBUG_LED
  if (hepd->led_present) gpio_set_level(hepd->led_pin, 0);
#endif
  cs_low(hepd);
  gpio_set_level(hepd->dc_pin, 0);                // command
  esp_err_t err = spi_tx(hepd, &command, 1);
  if (err != ESP_OK) { cs_high(hepd); goto out; }

  gpio_set_level(hepd->dc_pin, 1);                // data
  if (size > 0 && pData)
    err = spi_tx(hepd, pData, size);

  cs_high(hepd);
out:
#ifdef CONFIG_SSD1680_DEBUG_LED
  if (hepd->led_present) gpio_set_level(hepd->led_pin, 1);
#endif
  return err;
}

static esp_err_t SSD1680_Receive(SSD1680_HandleTypeDef *hepd, const uint8_t command, uint8_t *pData, size_t size)
{
#ifdef CONFIG_SSD1680_DEBUG_LED
  if (hepd->led_present) gpio_set_level(hepd->led_pin, 0);
#endif
  cs_low(hepd);
  gpio_set_level(hepd->dc_pin, 0);                // command
  esp_err_t err = spi_tx(hepd, &command, 1);
  if (err != ESP_OK) { cs_high(hepd); goto out; }

  gpio_set_level(hepd->dc_pin, 1);                // data
  if (size > 0 && pData)
    err = spi_rx(hepd, pData, size);

  cs_high(hepd);
out:
#ifdef CONFIG_SSD1680_DEBUG_LED
  if (hepd->led_present) gpio_set_level(hepd->led_pin, 1);
#endif
  return err;
}

// --- GPIO/BUSY/RESET
void SSD1680_Reset(SSD1680_HandleTypeDef *hepd)
{
  gpio_set_level(hepd->reset_pin, 0);
  delay_ms(2);
  gpio_set_level(hepd->reset_pin, 1);
  delay_ms(10);
}

static void SSD1680_Wait(SSD1680_HandleTypeDef *hepd)
{
  while (gpio_get_level(hepd->busy_pin) == 1) {
    delay_ms(2);
  }
}

// --- mid-level helpers (ported 1:1)
static esp_err_t SSD1680_GateScanRange(SSD1680_HandleTypeDef *hepd, uint16_t top, uint16_t height)
{
  esp_err_t err = SSD1680_Send(hepd, SSD1680_GATE_SCAN_START, (uint8_t*)&top, sizeof(top));
  if (err != ESP_OK) return err;

#pragma pack(push,1)
  struct { uint16_t height; uint8_t order; } gateScan = { height, 0 };
#pragma pack(pop)
  return SSD1680_Send(hepd, SSD1680_GATE_SCAN, (uint8_t*)&gateScan, sizeof(gateScan));
}

static esp_err_t SSD1680_DataEntryMode_Set(SSD1680_HandleTypeDef *hepd, enum SSD1680_DataEntryMode mode)
{
  uint8_t v = (uint8_t)mode;
  return SSD1680_Send(hepd, SSD1680_DATA_ENTRY_MODE, &v, sizeof(v));
}

static esp_err_t SSD1680_UpdateControl(SSD1680_HandleTypeDef *hepd)
{
  const uint8_t inverseR = 0;
  const uint8_t bypassR  = hepd->Color_Depth & 0x01;
  const uint8_t inverseK = 0;
  const uint8_t bypassK  = 0;

#pragma pack(push,1)
  struct {
    uint8_t reserved1:2;
    uint8_t bypassK:1;
    uint8_t inverseK:1;
    uint8_t reserved2:2;
    uint8_t bypassR:1;
    uint8_t inverseR:1;
    uint8_t reserved0:7;
    uint8_t sourceMode:1;
  } updateControl1 = { 0, bypassK, inverseK, 0, bypassR, inverseR, 0, hepd->Scan_Mode };
#pragma pack(pop)

  return SSD1680_Send(hepd, SSD1680_UPDATE_CONTROL_1, (uint8_t*)&updateControl1, sizeof(updateControl1));
}

// --- public init (same seq, HAL→ESP-IDF)
void SSD1680_Init(SSD1680_HandleTypeDef *hepd)
{
  // Ensure idle states
  gpio_set_level(hepd->reset_pin, 0);
  cs_high(hepd);
  gpio_set_level(hepd->dc_pin, 1);
  delay_ms(2);
  gpio_set_level(hepd->reset_pin, 1);
  delay_ms(20);

  SSD1680_Reset(hepd);
  SSD1680_Send(hepd, SSD1680_SW_RESET, NULL, 0);
  SSD1680_Wait(hepd);

  uint8_t userId[10] = {0};
  SSD1680_Receive(hepd, SSD1680_READ_USER_ID, userId, sizeof(userId));

  SSD1680_GateScanRange(hepd, 0, hepd->Resolution_Y);
  SSD1680_UpdateControl(hepd);
  SSD1680_DataEntryMode_Set(hepd, RightThenDown);

  // Write temperature 0x64 and load it
  const uint8_t temp[] = { 0x64 };
  SSD1680_Send(hepd, SSD1680_WRITE_TEMP, temp, sizeof(temp));
  const uint8_t temp2[] = { 0x91 };
  SSD1680_Send(hepd, SSD1680_UPDATE_CONTROL_2, temp2, sizeof(temp2));
  SSD1680_Send(hepd, SSD1680_MASTER_ACTIVATION, NULL, 0);
  SSD1680_Wait(hepd);
}

// --- Ranges / addressing
static esp_err_t SSD1680_RAMXRange(SSD1680_HandleTypeDef *hepd, uint8_t left, uint8_t width)
{
  if ((left % 8) || (width % 8)) return ESP_FAIL;
  uint8_t ramXRange[] = { (uint8_t)(left / 8), (uint8_t)((left + width) / 8 - 1) };
  return SSD1680_Send(hepd, SSD1680_RAM_X_RANGE, ramXRange, sizeof(ramXRange));
}

static esp_err_t SSD1680_RAMYRange(SSD1680_HandleTypeDef *hepd, uint16_t top, uint16_t height)
{
  uint16_t ramYRange[] = { top, (uint16_t)(top + height - 1) };
  return SSD1680_Send(hepd, SSD1680_RAM_Y_RANGE, (uint8_t*)ramYRange, sizeof(ramYRange));
}

static esp_err_t SSD1680_StartAddress(SSD1680_HandleTypeDef *hepd, uint8_t x, uint16_t y)
{
  if (x % 8) return ESP_FAIL;
  uint8_t xaddr = x / 8;
  esp_err_t err = SSD1680_Send(hepd, SSD1680_RAM_X, &xaddr, sizeof(xaddr));
  if (err != ESP_OK) return err;
  return SSD1680_Send(hepd, SSD1680_RAM_Y, (uint8_t*)&y, sizeof(y));
}

// --- Patterns & clear
static esp_err_t SSD1680_RAMReadOption(SSD1680_HandleTypeDef *hepd, enum SSD1680_RAMBank ram)
{
  uint8_t v = (uint8_t)ram;
  return SSD1680_Send(hepd, SSD1680_RAM_READ_OPT, &v, 1);
}

static esp_err_t SSD1680_RAMFill(SSD1680_HandleTypeDef *hepd,
                                 enum SSD1680_Pattern kx, enum SSD1680_Pattern ky,
                                 enum SSD1680_Pattern rx, enum SSD1680_Pattern ry,
                                 enum SSD1680_Color color)
{
  esp_err_t err = SSD1680_RAMXRange(hepd, 0, hepd->Resolution_X);
  if (err!=ESP_OK) return err;
  err = SSD1680_RAMYRange(hepd, 0, hepd->Resolution_Y);
  if (err!=ESP_OK) return err;

  uint8_t pattern = ((uint8_t)ky << 4) | (uint8_t)kx | (((uint8_t)color & 1) << 7);
  err = SSD1680_Send(hepd, SSD1680_PATTERN_BLACK, &pattern, sizeof(pattern));
  if (err!=ESP_OK) return err;
  SSD1680_Wait(hepd);

  pattern = ((uint8_t)ry << 4) | (uint8_t)rx | (((uint8_t)color & 2) << 6);
  err = SSD1680_Send(hepd, SSD1680_PATTERN_RED, &pattern, sizeof(pattern));
  if (err!=ESP_OK) return err;
  SSD1680_Wait(hepd);
  return ESP_OK;
}

esp_err_t SSD1680_Clear(SSD1680_HandleTypeDef *hepd, enum SSD1680_Color color)
{
  return SSD1680_RAMFill(hepd, PatternSolid, PatternSolid, PatternSolid, PatternSolid, color);
}

esp_err_t SSD1680_Checker(SSD1680_HandleTypeDef *hepd)
{
  return SSD1680_RAMFill(hepd, Pattern16, Pattern16, Pattern8, Pattern8, ColorAnotherRed);
}

// --- Refresh / Border
esp_err_t SSD1680_Refresh(SSD1680_HandleTypeDef *hepd, enum SSD1680_RefreshMode mode)
{
  const uint8_t boosterSoftStart[] = { 0x80, 0x90, 0x90, 0x00 };
  esp_err_t err = SSD1680_Send(hepd, SSD1680_BOOSTER_SOFT_START, boosterSoftStart, sizeof(boosterSoftStart));
  if (err!=ESP_OK) return err;

  uint8_t m = (uint8_t)mode;
  err = SSD1680_Send(hepd, SSD1680_UPDATE_CONTROL_2, &m, sizeof(m));
  if (err!=ESP_OK) return err;

  err = SSD1680_Send(hepd, SSD1680_MASTER_ACTIVATION, NULL, 0);
  if (err!=ESP_OK) return err;

  SSD1680_Wait(hepd);
  return ESP_OK;
}

esp_err_t SSD1680_Border(SSD1680_HandleTypeDef *hepd, enum SSD1680_Color color)
{
#pragma pack(push,1)
  struct {
    uint8_t LUT:2;
    uint8_t transition:1;
    uint8_t reserved:1;
    uint8_t level:2;
    uint8_t mode:2;
  } border = { color, 1, 0, 0, 0 };
#pragma pack(pop)
  return SSD1680_Send(hepd, SSD1680_BORDER, (uint8_t*)&border, sizeof(border));
}

// --- Region read/write
esp_err_t SSD1680_GetRegion(SSD1680_HandleTypeDef *hepd, uint8_t left, uint16_t top, uint8_t width, uint16_t height,
                            uint8_t *data_k, uint8_t *data_r)
{
  esp_err_t err = SSD1680_RAMXRange(hepd, left, width);
  if (err!=ESP_OK) return err;
  err = SSD1680_RAMYRange(hepd, top, height);
  if (err!=ESP_OK) return err;

  size_t bytes = (width / 8) * height;

  if (data_k) {
    err = SSD1680_RAMReadOption(hepd, RAMBlack);
    if (err!=ESP_OK) return err;
    err = SSD1680_StartAddress(hepd, left, top);
    if (err!=ESP_OK) return err;

    // Send READ command
    cs_low(hepd);
    gpio_set_level(hepd->dc_pin, 0);
    uint8_t cmd = SSD1680_READ;
    err = spi_tx(hepd, &cmd, 1);
    if (err!=ESP_OK) { cs_high(hepd); return err; }
    gpio_set_level(hepd->dc_pin, 1);

#if SSD1680_DUMMY_BYTES
    uint8_t dummy[SSD1680_DUMMY_BYTES];
    err = spi_rx(hepd, dummy, sizeof(dummy));
    if (err!=ESP_OK) { cs_high(hepd); return err; }
#endif
    err = spi_rx(hepd, data_k, bytes);
    cs_high(hepd);
    if (err!=ESP_OK) return err;
  }

  // (Optional) Red bank read could be added similarly if controller supports/needs a selector; many panels don’t support RAM read on red.

  return ESP_OK;
}

esp_err_t SSD1680_SetRegion(SSD1680_HandleTypeDef *hepd, uint8_t left, uint16_t top, uint8_t width, uint16_t height,
                            const uint8_t *data_k, const uint8_t *data_r)
{
  esp_err_t err = SSD1680_RAMXRange(hepd, left, width);
  if (err!=ESP_OK) return err;
  err = SSD1680_RAMYRange(hepd, top, height);
  if (err!=ESP_OK) return err;

  size_t bytes = (width / 8) * height;

  if (data_k) {
    err = SSD1680_StartAddress(hepd, left, top);
    if (err!=ESP_OK) return err;
    err = SSD1680_Send(hepd, SSD1680_WRITE_BLACK, data_k, bytes);
    if (err!=ESP_OK) return err;
  }
  if (data_r) {
    err = SSD1680_StartAddress(hepd, left, top);
    if (err!=ESP_OK) return err;
    err = SSD1680_Send(hepd, SSD1680_WRITE_RED, data_r, bytes);
    if (err!=ESP_OK) return err;
  }
  return ESP_OK;
}

// --- Convenience (fix): reset ranges to full screen
static esp_err_t SSD1680_ResetRange(SSD1680_HandleTypeDef *hepd)
{
  esp_err_t err = SSD1680_RAMXRange(hepd, 0, hepd->Resolution_X); // << corrected (pixels)
  if (err!=ESP_OK) return err;
  return SSD1680_RAMYRange(hepd, 0, hepd->Resolution_Y);
}
