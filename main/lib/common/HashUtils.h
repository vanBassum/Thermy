#pragma once
#include <stdint.h>
#include <stddef.h>

namespace HashUtils
{
    // Simple, fast, deterministic 32-bit hash (FNV-1a)
    inline uint32_t FastHash(const void *data, size_t len)
    {
        const uint8_t *bytes = reinterpret_cast<const uint8_t *>(data);
        uint32_t hash = 2166136261u;  // FNV offset basis
        for (size_t i = 0; i < len; ++i)
        {
            hash ^= bytes[i];
            hash *= 16777619u; // FNV prime
        }
        return hash;
    }

    // Optional helper overload for structs
    template<typename T>
    inline uint32_t FastHash(const T &obj)
    {
        return FastHash(&obj, sizeof(T));
    }
}
