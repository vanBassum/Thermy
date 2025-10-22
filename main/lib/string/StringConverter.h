#pragma once
#include <cstdint>
#include <cstddef>

class StringConverter
{
public:
    static const StringConverter& Default();
    
    // ----- FLOAT -----
    bool StringToFloat(float& value, const char* str) const;
    bool FloatToString(char* buffer, size_t size, float value) const;

    // ----- BOOL -----
    bool StringToBool(bool& value, const char* str) const;
    bool BoolToString(char* buffer, size_t size, bool value) const;

    // ----- INTEGERS -----
    bool StringToInt8(int8_t& value, const char* str) const;
    bool Int8ToString(char* buffer, size_t size, int8_t value) const;

    bool StringToUInt8(uint8_t& value, const char* str) const;
    bool UInt8ToString(char* buffer, size_t size, uint8_t value) const;

    bool StringToInt16(int16_t& value, const char* str) const;
    bool Int16ToString(char* buffer, size_t size, int16_t value) const;

    bool StringToUInt16(uint16_t& value, const char* str) const;
    bool UInt16ToString(char* buffer, size_t size, uint16_t value) const;

    bool StringToInt32(int32_t& value, const char* str) const;
    bool Int32ToString(char* buffer, size_t size, int32_t value) const;

    bool StringToUInt32(uint32_t& value, const char* str) const;
    bool UInt32ToString(char* buffer, size_t size, uint32_t value) const;

    bool StringToInt64(int64_t& value, const char* str) const;
    bool Int64ToString(char* buffer, size_t size, int64_t value) const;

    bool StringToUInt64(uint64_t& value, const char* str) const;
    bool UInt64ToString(char* buffer, size_t size, uint64_t value) const;

    // ----- ENUM -----
    template <typename TEnum>
    bool StringToEnum(TEnum& value, const char* str) const;

    template <typename TEnum>
    bool EnumToString(char* buffer, size_t size, TEnum value) const;

    // ----- BLOB -----
    bool BlobToString(char* buffer, size_t bufferSize, const void* data, size_t dataLen) const;
    bool StringToBlob(void* outData, size_t outDataSize, const char* str) const;

private:
    // --- Internal helpers ---
    bool ParseSigned(long& out, const char* str, long min, long max) const;
    bool ParseUnsigned(unsigned long& out, const char* str, unsigned long max) const;
    bool ParseSigned64(long long& out, const char* str, long long min, long long max) const;
    bool ParseUnsigned64(unsigned long long& out, const char* str, unsigned long long max) const;
};

// --- Template implementations (must remain in header) ---
template <typename TEnum>
bool StringConverter::StringToEnum(TEnum& value, const char* str) const
{
    long long intValue;
    if (!ParseSigned64(intValue, str,
                       std::numeric_limits<long long>::min(),
                       std::numeric_limits<long long>::max()))
        return false;
    value = static_cast<TEnum>(intValue);
    return true;
}

template <typename TEnum>
bool StringConverter::EnumToString(char* buffer, size_t size, TEnum value) const
{
    return std::snprintf(buffer, size, "%lld", static_cast<long long>(value)) > 0;
}
