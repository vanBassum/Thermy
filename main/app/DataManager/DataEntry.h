#pragma once
#include "DateTime.h"
#include "DataPair.h"
#include "HandledFlags.h"


struct DataEntry
{
    constexpr static const size_t PAIR_COUNT = 4;
    DateTime timestamp = DateTime::MinValue();
    DataPair pairs[PAIR_COUNT] = {};
    HandledFlags flags = HandledFlags::None;
};

