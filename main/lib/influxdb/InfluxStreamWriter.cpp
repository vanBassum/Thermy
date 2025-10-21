#include "InfluxStreamWriter.h"
#include <cstdio>
#include <cstring>

// --- Primitive writers ---

void InfluxStreamWriter::writeInt8(std::int8_t value)
{
    char buf[8];
    int len = std::snprintf(buf, sizeof(buf), "%d", static_cast<int>(value));
    _stream.write(buf, static_cast<std::size_t>(len));
}

void InfluxStreamWriter::writeInt16(std::int16_t value)
{
    char buf[12];
    int len = std::snprintf(buf, sizeof(buf), "%d", static_cast<int>(value));
    _stream.write(buf, static_cast<std::size_t>(len));
}

void InfluxStreamWriter::writeInt32(std::int32_t value)
{
    char buf[16];
    int len = std::snprintf(buf, sizeof(buf), "%ld", static_cast<long>(value));
    _stream.write(buf, static_cast<std::size_t>(len));
}

void InfluxStreamWriter::writeInt64(std::int64_t value)
{
    char buf[24];
    int len = std::snprintf(buf, sizeof(buf), "%lld", static_cast<long long>(value));
    _stream.write(buf, static_cast<std::size_t>(len));
}

void InfluxStreamWriter::writeUInt8(std::uint8_t value)
{
    char buf[8];
    int len = std::snprintf(buf, sizeof(buf), "%u", static_cast<unsigned int>(value));
    _stream.write(buf, static_cast<std::size_t>(len));
}

void InfluxStreamWriter::writeUInt16(std::uint16_t value)
{
    char buf[12];
    int len = std::snprintf(buf, sizeof(buf), "%u", static_cast<unsigned int>(value));
    _stream.write(buf, static_cast<std::size_t>(len));
}

void InfluxStreamWriter::writeUInt32(std::uint32_t value)
{
    char buf[16];
    int len = std::snprintf(buf, sizeof(buf), "%lu", static_cast<unsigned long>(value));
    _stream.write(buf, static_cast<std::size_t>(len));
}

void InfluxStreamWriter::writeUInt64(std::uint64_t value)
{
    char buf[24];
    int len = std::snprintf(buf, sizeof(buf), "%llu", static_cast<unsigned long long>(value));
    _stream.write(buf, static_cast<std::size_t>(len));
}

void InfluxStreamWriter::writeFloat(float value)
{
    char buf[24];
    int len = std::snprintf(buf, sizeof(buf), "%g", static_cast<double>(value));
    _stream.write(buf, static_cast<std::size_t>(len));
}

void InfluxStreamWriter::writeDouble(double value)
{
    char buf[32];
    int len = std::snprintf(buf, sizeof(buf), "%g", value);
    _stream.write(buf, static_cast<std::size_t>(len));
}

void InfluxStreamWriter::writeBool(bool value)
{
    const char* text = value ? "true" : "false";
    _stream.write(text, std::strlen(text));
}

// --- String writing ---
void InfluxStreamWriter::writeString(const char* s)
{
    if (!s) return;
    _stream.write(s, std::strlen(s));
}

// --- Escaping helpers ---
// For measurement/tag/field keys: escape ' ', ',', and '=' with '\'
void InfluxStreamWriter::writeEscapedKey(const char* key)
{
    if (!key) return;
    for (const char* p = key; *p; ++p)
    {
        if (*p == ' ' || *p == ',' || *p == '=')
        {
            const char backslash = '\\';
            _stream.write(&backslash, 1);
        }
        _stream.write(p, 1);
    }
}

// For string field values: escape '"' and '\' with '\'
void InfluxStreamWriter::writeEscapedValue(const char* value)
{
    if (!value) return;

    const char quote = '"';
    _stream.write(&quote, 1);

    for (const char* p = value; *p; ++p)
    {
        if (*p == '"' || *p == '\\')
        {
            const char backslash = '\\';
            _stream.write(&backslash, 1);
        }
        _stream.write(p, 1);
    }

    _stream.write(&quote, 1);
}
