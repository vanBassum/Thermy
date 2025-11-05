#pragma once
#include <stdint.h>   // for int32_t, etc.
#include <cstring>
#include <type_traits>

/* ---------------------------------------------
 * Basic macros
 * --------------------------------------------- */
#define ABS(x)          ((x) < 0 ? -(x) : (x))
#define MIN(a,b)        ((a) < (b) ? (a) : (b))
#define MAX(a,b)        ((a) > (b) ? (a) : (b))
#define CLAMP(x,lo,hi)  ((x) < (lo) ? (lo) : ((x) > (hi) ? (hi) : (x)))


/* ---------------------------------------------
 * Bit helpers
 * --------------------------------------------- */
#define BIT(n)          (1U << (n))
#define BIT_SET(v,n)    ((v) |= BIT(n))
#define BIT_CLEAR(v,n)  ((v) &= ~BIT(n))
#define BIT_TOGGLE(v,n) ((v) ^= BIT(n))
#define BIT_CHECK(v,n)  (((v) >> (n)) & 1U)

/* ---------------------------------------------
 * Array / size helpers
 * --------------------------------------------- */
#define COUNT_OF(arr)   (sizeof(arr) / sizeof((arr)[0]))
#define ARRAY_END(arr)  ((arr) + COUNT_OF(arr))



static bool IsElapsed(TickType_t now, TickType_t last, TickType_t interval)
{
    return (now - last) >= interval;
}

static TickType_t GetSleepTime(TickType_t now, TickType_t last, TickType_t interval)
{
    TickType_t diff = now - last;
    if (diff >= interval)
        return 0;
    return interval - diff;
}
