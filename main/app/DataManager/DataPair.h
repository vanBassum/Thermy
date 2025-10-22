#pragma once
#include <stdint.h>
#include <stddef.h>
#include "DataKey.h"

struct DataPair
{
    constexpr static const size_t VALUE_SIZE = 8;
    DataKey key = DataKey::Empty;
    uint8_t value[VALUE_SIZE] = {0};
};

