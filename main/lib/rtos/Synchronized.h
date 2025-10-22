#pragma once
#include "Mutex.h"
#include <type_traits>

template<typename T>
class Synchronized
{
    static_assert(
        std::is_enum_v<T> || std::is_integral_v<T> || std::is_floating_point_v<T>,
        "Synchronized<T> only supports simple types (enum, int, float, etc.)"
    );

public:
    constexpr Synchronized() = default;
    explicit constexpr Synchronized(T value) : _value(value) {}
    Synchronized(const Synchronized&) = delete;
    Synchronized& operator=(const Synchronized&) = delete;

    // Thread-safe set
    void Set(const T value)
    {
        LOCK(_mutex);
        _value = value;
    }

    // Thread-safe get (returns copy)
    T Get() const
    {
        LOCK(_mutex);
        return _value;
    }

private:
    Mutex _mutex;
    T _value{};
};
