#pragma once

#include <cstdint>

enum class LogKeys : uint8_t
{
    LogCode,
    TimeStamp,
    Temperature_1,
    Temperature_2,
    Temperature_3,
    Temperature_4,
    IpAddress,
    FirmwareVersion,
};

enum class LogCode : uint32_t
{
    // System events
    SystemBoot,
    TimeSynced,

    // Network events
    StaConnected,
    StaDisconnected,
    StaReconnecting,
    IpAcquired,
    IpLost,
    ApStarted,
    ApFallback,

    // Sensor events
    TemperatureReading,
};
