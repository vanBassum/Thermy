#pragma once

#include <stdint.h>
#include "esp_err.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"

// https://github.com/Erwin-Zhuang/SSD1680_Driver/blob/main/Src/SSD1680.c

#ifdef __cplusplus
extern "C" {
#endif

// --- Command set (unchanged)
#define SSD1680_GATE_SCAN 0x01
#define SSD1680_GATE_VOLTAGE 0x03
#define SSD1680_SOURCE_VOLTAGE 0x04
#define SSD1680_BOOSTER_SOFT_START 0x0C
#define SSD1680_GATE_SCAN_START 0x0F
#define SSD1680_DATA_ENTRY_MODE 0x11
#define SSD1680_SW_RESET 0x12
#define SSD1680_SELECT_TEMP_SENSOR 0x18
#define SSD1680_WRITE_TEMP 0x1A
#define SSD1680_READ_TEMP 0x1B
#define SSD1680_MASTER_ACTIVATION 0x20
#define SSD1680_UPDATE_CONTROL_1 0x21
#define SSD1680_UPDATE_CONTROL_2 0x22
#define SSD1680_WRITE_BLACK 0x24
#define SSD1680_WRITE_RED 0x26
#define SSD1680_READ 0x27
#define SSD1680_VCOM_VOLTAGE 0x2C
#define SSD1680_READ_USER_ID 0x2E
#define SSD1680_BORDER 0x3C
#define SSD1680_RAM_READ_OPT 0x41
#define SSD1680_RAM_X_RANGE 0x44
#define SSD1680_RAM_Y_RANGE 0x45
#define SSD1680_PATTERN_RED 0x46
#define SSD1680_PATTERN_BLACK 0x47
#define SSD1680_RAM_X 0x4E
#define SSD1680_RAM_Y 0x4F
#define SSD1680_NOP 0x7F

// --- Enums (unchanged)
enum SSD1680_Color { ColorBlack=0, ColorWhite, ColorRed, ColorAnotherRed };
enum SSD1680_Pattern { Pattern8=0, Pattern16, Pattern32, Pattern64, Pattern128, Pattern256, PatternSolid=7 };
enum SSD1680_ScanMode { WideScan=0, NarrowScan };
enum SSD1680_RAMBank { RAMBlack=0, RAMRed };
enum SSD1680_DataEntryMode {
  LeftThenUp=0, RightThenUp, LeftThenDown, RightThenDown,
  UpThenLeft, UpThenRight, DownThenLeft, DownThenRight
};
enum SSD1680_RefreshMode {
  FullRefresh=0xF7, PartialRefresh=0xFF, FastFullRefresh=0xC7, FastPartialRefresh=0xCF
};

// --- Handle: ESP-IDF flavored
typedef struct {
  spi_device_handle_t spi;      // SPI device
  int spi_timeout_ms;           // timeout for transactions
  gpio_num_t cs_pin;            // manual CS (set spics_io_num=-1 on device)
  gpio_num_t dc_pin;            // D/C
  gpio_num_t reset_pin;         // !RESET
  gpio_num_t busy_pin;          // BUSY (high=busy on most)
  uint8_t Color_Depth;          // 1 or 2 bits
  enum SSD1680_ScanMode Scan_Mode;
  uint8_t Resolution_X;         // pixels, multiple of 8
  uint16_t Resolution_Y;        // pixels
#ifdef CONFIG_SSD1680_DEBUG_LED
  gpio_num_t led_pin;           // optional activity LED
  bool led_present;
#endif
} SSD1680_HandleTypeDef;

// Connectivity
void SSD1680_Reset(SSD1680_HandleTypeDef *hepd);
void SSD1680_Init(SSD1680_HandleTypeDef *hepd);

// High-level
esp_err_t SSD1680_Clear(SSD1680_HandleTypeDef *hepd, const enum SSD1680_Color color);
esp_err_t SSD1680_Refresh(SSD1680_HandleTypeDef *hepd, const enum SSD1680_RefreshMode mode);
esp_err_t SSD1680_Border(SSD1680_HandleTypeDef *hepd, const enum SSD1680_Color color);
esp_err_t SSD1680_GetRegion(SSD1680_HandleTypeDef *hepd, const uint8_t left, const uint16_t top,
                            const uint8_t width, const uint16_t height, uint8_t *data_k, uint8_t *data_r);
esp_err_t SSD1680_SetRegion(SSD1680_HandleTypeDef *hepd, const uint8_t left, const uint16_t top,
                            const uint8_t width, const uint16_t height, const uint8_t *data_k, const uint8_t *data_r);
esp_err_t SSD1680_Checker(SSD1680_HandleTypeDef *hepd);

#ifdef __cplusplus
}
#endif
