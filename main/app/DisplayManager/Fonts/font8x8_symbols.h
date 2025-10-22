#pragma once
#include <stdint.h>
#include "FontDef.h"

enum class SymbolIcon : char
{
    Empty = 0,
    Wifi,
    Bluetooth,
    BatteryFull,
    BatteryLow,
    Thermometer,
    Cloud,
    Sun,
    ArrowUp,
};

static const uint8_t font8x8_symbols[9][8] = {
    // Empty (space)
    {0b00000000,
     0b00000000,
     0b00000000,
     0b00000000,
     0b00000000,
     0b00000000,
     0b00000000,
     0b00000000},

    // WiFi icon
    {0b00000100,
     0b00000010,
     0b00001000,
     0b01100101,
     0b01100101,
     0b00001000,
     0b00000010,
     0b00000100},

    // Bluetooth symbol
    {0b00010000,
     0b00011000,
     0b00010100,
     0b11110010,
     0b00010100,
     0b00011000,
     0b00010000,
     0b00000000},

    // Battery (full)
    {0b01111110,
     0b11111111,
     0b10000001,
     0b10111101,
     0b10111101,
     0b10000001,
     0b11111111,
     0b01111110},

    // Battery (low)
    {0b01111110,
     0b11111111,
     0b10000001,
     0b10001101,
     0b10001101,
     0b10000001,
     0b11111111,
     0b01111110},

    // Thermometer
    {0b00011000,
     0b00011000,
     0b00011000,
     0b00011000,
     0b00011000,
     0b00111100,
     0b00111100,
     0b00011000},

    // Cloud
    {0b00000000,
     0b00111000,
     0b01111100,
     0b11111110,
     0b11111110,
     0b01111100,
     0b00111000,
     0b00000000},

    // Sun
    {0b00100100,
     0b00011000,
     0b00111100,
     0b01111110,
     0b01111110,
     0b00111100,
     0b00011000,
     0b00100100},

    // Arrow up
    {0b00001000,
     0b00011100,
     0b00111110,
     0b01111111,
     0b00001000,
     0b00001000,
     0b00001000,
     0b00000000}};

static const FontDef Font8x8_Symbols = {
    .table = &font8x8_symbols[0][0],
    .width = 8,
    .height = 8,
    .firstChar = 0,
    .lastChar = 8,
};