#pragma once
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <strings.h>
#include <cerrno>
#include <limits>

class StringConverter
{
public:
    // ----- FLOAT -----
    bool StringToFloat(float &value, const char *str) const
    {
        if (!str)
            return false;
        char *end;
        errno = 0;
        value = strtof(str, &end);
        return end != str && errno == 0;
    }

    bool FloatToString(char *buffer, size_t size, float value) const
    {
        return snprintf(buffer, size, "%.2f", value) > 0;
    }

    // ----- BOOL -----
    bool StringToBool(bool &value, const char *str) const
    {
        if (!str)
            return false;
        if (strcasecmp(str, "true") == 0 || strcmp(str, "1") == 0)
        {
            value = true;
            return true;
        }
        if (strcasecmp(str, "false") == 0 || strcmp(str, "0") == 0)
        {
            value = false;
            return true;
        }
        return false;
    }

    bool BoolToString(char *buffer, size_t size, bool value) const
    {
        return snprintf(buffer, size, "%s", value ? "true" : "false") > 0;
    }

    // ----- INT8 -----
    bool StringToInt8(int8_t &value, const char *str) const
    {
        long val;
        if (!ParseSigned(val, str, std::numeric_limits<int8_t>::min(), std::numeric_limits<int8_t>::max()))
            return false;
        value = static_cast<int8_t>(val);
        return true;
    }

    bool Int8ToString(char *buffer, size_t size, int8_t value) const
    {
        return snprintf(buffer, size, "%d", static_cast<int>(value)) > 0;
    }

    // ----- UINT8 -----
    bool StringToUInt8(uint8_t &value, const char *str) const
    {
        unsigned long val;
        if (!ParseUnsigned(val, str, std::numeric_limits<uint8_t>::max()))
            return false;
        value = static_cast<uint8_t>(val);
        return true;
    }

    bool UInt8ToString(char *buffer, size_t size, uint8_t value) const
    {
        return snprintf(buffer, size, "%u", static_cast<unsigned int>(value)) > 0;
    }

    // ----- INT16 -----
    bool StringToInt16(int16_t &value, const char *str) const
    {
        long val;
        if (!ParseSigned(val, str, std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::max()))
            return false;
        value = static_cast<int16_t>(val);
        return true;
    }

    bool Int16ToString(char *buffer, size_t size, int16_t value) const
    {
        return snprintf(buffer, size, "%d", static_cast<int>(value)) > 0;
    }

    // ----- UINT16 -----
    bool StringToUInt16(uint16_t &value, const char *str) const
    {
        unsigned long val;
        if (!ParseUnsigned(val, str, std::numeric_limits<uint16_t>::max()))
            return false;
        value = static_cast<uint16_t>(val);
        return true;
    }

    bool UInt16ToString(char *buffer, size_t size, uint16_t value) const
    {
        return snprintf(buffer, size, "%u", static_cast<unsigned int>(value)) > 0;
    }

    // ----- INT32 -----
    bool StringToInt32(int32_t &value, const char *str) const
    {
        long val;
        if (!ParseSigned(val, str, std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max()))
            return false;
        value = static_cast<int32_t>(val);
        return true;
    }

    bool Int32ToString(char *buffer, size_t size, int32_t value) const
    {
        return snprintf(buffer, size, "%ld", static_cast<long>(value)) > 0;
    }

    // ----- UINT32 -----
    bool StringToUInt32(uint32_t &value, const char *str) const
    {
        unsigned long val;
        if (!ParseUnsigned(val, str, std::numeric_limits<uint32_t>::max()))
            return false;
        value = static_cast<uint32_t>(val);
        return true;
    }

    bool UInt32ToString(char *buffer, size_t size, uint32_t value) const
    {
        return snprintf(buffer, size, "%lu", static_cast<unsigned long>(value)) > 0;
    }

    // ----- INT64 -----
    bool StringToInt64(int64_t &value, const char *str) const
    {
        long long val;
        if (!ParseSigned64(val, str, std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::max()))
            return false;
        value = static_cast<int64_t>(val);
        return true;
    }

    bool Int64ToString(char *buffer, size_t size, int64_t value) const
    {
        return snprintf(buffer, size, "%lld", static_cast<long long>(value)) > 0;
    }

    // ----- UINT64 -----
    bool StringToUInt64(uint64_t &value, const char *str) const
    {
        unsigned long long val;
        if (!ParseUnsigned64(val, str, std::numeric_limits<uint64_t>::max()))
            return false;
        value = static_cast<uint64_t>(val);
        return true;
    }

    bool UInt64ToString(char *buffer, size_t size, uint64_t value) const
    {
        return snprintf(buffer, size, "%llu", static_cast<unsigned long long>(value)) > 0;
    }

    // ----- ENUM -----
    template <typename TEnum>
    bool StringToEnum(TEnum &value, const char *str) const
    {
        long long intValue;
        if (!ParseSigned64(intValue, str, std::numeric_limits<long long>::min(), std::numeric_limits<long long>::max()))
            return false;
        value = static_cast<TEnum>(intValue);
        return true;
    }

    template <typename TEnum>
    bool EnumToString(char *buffer, size_t size, TEnum value) const
    {
        return snprintf(buffer, size, "%lld", static_cast<long long>(value)) > 0;
    }

    // ----- BLOB (uint8_t[]) -----
    bool BlobToString(char *buffer, size_t bufferSize, const void *data, size_t dataLen) const
    {
        if (!buffer || !data || bufferSize == 0)
            return false;

        // Need 2 hex chars per byte + null terminator
        if (bufferSize < dataLen * 2 + 1)
            return false;

        const uint8_t *byteData = static_cast<const uint8_t *>(data);

        for (size_t i = 0; i < dataLen; ++i)
            snprintf(buffer + i * 2, 3, "%02X", byteData[i]);

        buffer[dataLen * 2] = '\0';
        return true;
    }

    bool StringToBlob(void *outData, size_t outDataSize, const char *str) const
    {
        if (!outData || !str)
            return false;

        // Fill everything with 0x00 first
        memset(outData, 0x00, outDataSize);

        size_t strLen = strlen(str);
        if (strLen == 0)
            return true; // empty string → all zeros

        // Must have even number of hex digits
        if (strLen % 2 != 0)
            return false;

        size_t byteCount = strLen / 2;
        if (byteCount > outDataSize)
            byteCount = outDataSize; // truncate safely instead of failing

        uint8_t *outDataBytes = static_cast<uint8_t *>(outData);

        for (size_t i = 0; i < byteCount; ++i)
        {
            unsigned int byteVal;
            if (sscanf(str + i * 2, "%2X", &byteVal) != 1)
                return false;

            outDataBytes[i] = static_cast<uint8_t>(byteVal);
        }

        return true;
    }

private:
    // --- Helpers ---
    bool ParseSigned(long &out, const char *str, long min, long max) const
    {
        if (!str)
            return false;
        char *end;
        errno = 0;
        long val = strtol(str, &end, 10);
        if (end == str || errno != 0 || val < min || val > max)
            return false;
        out = val;
        return true;
    }

    bool ParseUnsigned(unsigned long &out, const char *str, unsigned long max) const
    {
        if (!str)
            return false;
        char *end;
        errno = 0;
        unsigned long val = strtoul(str, &end, 10);
        if (end == str || errno != 0 || val > max)
            return false;
        out = val;
        return true;
    }

    bool ParseSigned64(long long &out, const char *str, long long min, long long max) const
    {
        if (!str)
            return false;
        char *end;
        errno = 0;
        long long val = strtoll(str, &end, 10);
        if (end == str || errno != 0 || val < min || val > max)
            return false;
        out = val;
        return true;
    }

    bool ParseUnsigned64(unsigned long long &out, const char *str, unsigned long long max) const
    {
        if (!str)
            return false;
        char *end;
        errno = 0;
        unsigned long long val = strtoull(str, &end, 10);
        if (end == str || errno != 0 || val > max)
            return false;
        out = val;
        return true;
    }
};
