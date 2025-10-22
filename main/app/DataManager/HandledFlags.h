#pragma once
#include "EnumFlags.h"

enum class HandledFlags
{
    None = 0,
    WrittenToInflux = 1 << 0
};

ENABLE_BITMASK_OPERATORS(HandledFlags);