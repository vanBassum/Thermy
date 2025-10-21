#include "InfluxStreamWriter.h"
#include <cstdio>
#include <cstring>

InfluxStreamWriter::InfluxStreamWriter(Stream &stream, const char *name, const DateTime &timestamp)
    : _stream(stream),
      _hasTags(false),
      _hasFields(false),
      _timestamp(timestamp)
{
    // Start measurement
    writeEscaped(name);
}

void InfluxStreamWriter::writeEscaped(const char* s)
{
    // Very simple escape (Influx requires commas and spaces to be escaped)
    for (const char* p = s; *p; ++p) {
        if (*p == ' ' || *p == ',' || *p == '=')
            _stream.write("\\", 1);
        _stream.write(p, 1);
    }
}

void InfluxStreamWriter::writeKeyValue(const char* key, const char* value)
{
    writeEscaped(key);
    _stream.write("=", 1);
    _stream.write(value, std::strlen(value));
}



// --- Tags ---
InfluxStreamWriter& InfluxStreamWriter::withTag(const char* key, const char* value)
{
    _stream.write(_hasTags ? "," : ",", 1);
    writeKeyValue(key, value);
    _hasTags = true;
    return *this;
}

#define DEFINE_TAG_NUMERIC(type) \
InfluxStreamWriter& InfluxStreamWriter::withTag(const char* key, type value) { \
    char buf[32]; snprintf(buf, sizeof(buf), "%lld", (long long)value); \
    return withTag(key, buf); \
}

DEFINE_TAG_NUMERIC(std::int8_t)
DEFINE_TAG_NUMERIC(std::int16_t)
DEFINE_TAG_NUMERIC(std::int32_t)
DEFINE_TAG_NUMERIC(std::int64_t)
DEFINE_TAG_NUMERIC(std::uint8_t)
DEFINE_TAG_NUMERIC(std::uint16_t)
DEFINE_TAG_NUMERIC(std::uint32_t)
DEFINE_TAG_NUMERIC(std::uint64_t)

// --- Fields ---
InfluxStreamWriter& InfluxStreamWriter::withField(const char* key, const char* value)
{
    if (!_hasFields) {
        _stream.write(" ", 1);
        _hasFields = true;
    } else {
        _stream.write(",", 1);
    }

    writeEscaped(key);
    _stream.write("=", 1);

    // Quote strings
    _stream.write("\"", 1);
    _stream.write(value, std::strlen(value));
    _stream.write("\"", 1);
    return *this;
}

#define DEFINE_FIELD_NUMERIC(type, fmt) \
InfluxStreamWriter& InfluxStreamWriter::withField(const char* key, type value) { \
    if (!_hasFields) { _stream.write(" ", 1); _hasFields = true; } else { _stream.write(",", 1); } \
    writeEscaped(key); \
    char buf[32]; snprintf(buf, sizeof(buf), fmt, value); \
    _stream.write("=", 1); \
    _stream.write(buf, std::strlen(buf)); \
    return *this; \
}

DEFINE_FIELD_NUMERIC(std::int8_t, "%d")
DEFINE_FIELD_NUMERIC(std::int16_t, "%d")
DEFINE_FIELD_NUMERIC(std::int32_t, "%ld")
DEFINE_FIELD_NUMERIC(std::int64_t, "%lld")
DEFINE_FIELD_NUMERIC(std::uint8_t, "%u")
DEFINE_FIELD_NUMERIC(std::uint16_t, "%u")
DEFINE_FIELD_NUMERIC(std::uint32_t, "%lu")
DEFINE_FIELD_NUMERIC(std::uint64_t, "%llu")
DEFINE_FIELD_NUMERIC(float, "%g")
DEFINE_FIELD_NUMERIC(double, "%g")

InfluxStreamWriter& InfluxStreamWriter::withField(const char* key, bool value)
{
    return withField(key, value ? "true" : "false");
}

// --- New measurement ---
InfluxStreamWriter& InfluxStreamWriter::withMeasurement(const char* name, const DateTime& timestamp)
{
    // Finish previous line
    Finish();
    _hasTags = false;
    _hasFields = false;
    _timestamp = timestamp;
    writeEscaped(name);
    return *this;
}

// --- Finish ---
void InfluxStreamWriter::Finish()
{
    char buf[32];
    int len = snprintf(buf, sizeof(buf), " %lld\n", _timestamp.UtcSeconds());
    _stream.write(buf, len);
}

