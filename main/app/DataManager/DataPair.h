#pragma once
#include <stdint.h>
#include <stddef.h>
#include "DataKey.h"
#include "LogCode.h"

struct DataPair
{
    DataKey key = DataKey::Empty;

    union
    {
        float   asFloat;
        uint32_t asUint32;
        uint64_t asUint64;
        int32_t  asInt32;
        int64_t  asInt64;
        LogCode asLogCode;
    } value;

    DataPair() { memset(&value, 0, sizeof(value)); }
    explicit DataPair(DataKey k) : key(k) { memset(&value, 0, sizeof(value)); }
    DataPair(DataKey k, float f) : key(k) { value.asFloat = f; }
    DataPair(DataKey k, uint64_t u64) : key(k) { value.asUint64 = u64; }
    DataPair(DataKey k, LogCode lCode) : key(k) { value.asLogCode = lCode; }
};
