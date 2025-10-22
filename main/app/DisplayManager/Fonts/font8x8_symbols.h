#pragma once
#include <stdint.h>
#include "FontDef.h"

enum class SymbolIcon : char
{
    Empty = 0,
    Wifi,
    NtpSynced,
};

static const uint8_t font8x8_symbols[3][8] = {
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

    // NTP synced icon (clock with check mark)
    {0b00011100,
     0b00100010,
     0b01000101,
     0b01001001,
     0b01010001,
     0b00100010,
     0b00011100,
     0b00000100},
};

static const FontDef Font8x8_Symbols = {
    .table = &font8x8_symbols[0][0],
    .width = 8,
    .height = 8,
    .firstChar = 0,
    .lastChar = 2,
};
