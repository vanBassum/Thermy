#include "InfluxMeasurementStreamWriter.h"
#include "InfluxClient.h"
#include <cstdio>
#include <cstring>

InfluxMeasurementStreamWriter::InfluxMeasurementStreamWriter(RequestStream& stream,
                                                             InfluxClient& client,
                                                             const char* name,
                                                             std::int64_t timestamp)
    : _stream(stream),
      _client(client),
      _writer(stream),
      _currentName(name),
      _currentTimestamp(timestamp),
      _hasFields(false),
      _hasTags(false)
{
    _writer.writeEscapedKey(name);
}

// ========== TAGS ==========

InfluxMeasurementStreamWriter& InfluxMeasurementStreamWriter::withTag(const char* key, const char* value)
{
    _stream.write(",", 1);
    _writer.writeEscapedKey(key);
    _stream.write("=", 1);
    _writer.writeEscapedValue(value);
    _hasTags = true;
    return *this;
}

#define DEFINE_TAG_INT(fnType, fmt) \
InfluxMeasurementStreamWriter& InfluxMeasurementStreamWriter::withTag(const char* key, fnType value) { \
    char buf[32]; \
    int len = std::snprintf(buf, sizeof(buf), fmt, static_cast<long long>(value)); \
    _stream.write(",", 1); \
    _writer.writeEscapedKey(key); \
    _stream.write("=", 1); \
    _stream.write(buf, static_cast<std::size_t>(len)); \
    _hasTags = true; \
    return *this; \
}

DEFINE_TAG_INT(std::int8_t, "%lld")
DEFINE_TAG_INT(std::int16_t, "%lld")
DEFINE_TAG_INT(std::int32_t, "%lld")
DEFINE_TAG_INT(std::int64_t, "%lld")
DEFINE_TAG_INT(std::uint8_t, "%llu")
DEFINE_TAG_INT(std::uint16_t, "%llu")
DEFINE_TAG_INT(std::uint32_t, "%llu")
DEFINE_TAG_INT(std::uint64_t, "%llu")

#undef DEFINE_TAG_INT

// ========== FIELDS ==========

void InfluxMeasurementStreamWriter::beginField(const char* key)
{
    if (!_hasFields) {
        _stream.write(" ", 1);
        _hasFields = true;
    } else {
        _stream.write(",", 1);
    }
    _writer.writeEscapedKey(key);
    _stream.write("=", 1);
}

InfluxMeasurementStreamWriter& InfluxMeasurementStreamWriter::withField(const char* key, const char* value)
{
    beginField(key);
    _writer.writeEscapedValue(value);
    return *this;
}

// integer helpers
void InfluxMeasurementStreamWriter::writeIntField(std::int64_t value)
{
    char buf[32];
    int len = std::snprintf(buf, sizeof(buf), "%lldi", static_cast<long long>(value));
    _stream.write(buf, static_cast<std::size_t>(len));
}

void InfluxMeasurementStreamWriter::writeUIntField(std::uint64_t value)
{
    char buf[32];
    int len = std::snprintf(buf, sizeof(buf), "%llui", static_cast<unsigned long long>(value));
    _stream.write(buf, static_cast<std::size_t>(len));
}

#define DEFINE_FIELD_INT(fnType) \
InfluxMeasurementStreamWriter& InfluxMeasurementStreamWriter::withField(const char* key, fnType value) { \
    beginField(key); writeIntField(static_cast<std::int64_t>(value)); return *this; }

#define DEFINE_FIELD_UINT(fnType) \
InfluxMeasurementStreamWriter& InfluxMeasurementStreamWriter::withField(const char* key, fnType value) { \
    beginField(key); writeUIntField(static_cast<std::uint64_t>(value)); return *this; }

DEFINE_FIELD_INT(std::int8_t)
DEFINE_FIELD_INT(std::int16_t)
DEFINE_FIELD_INT(std::int32_t)
DEFINE_FIELD_INT(std::int64_t)

DEFINE_FIELD_UINT(std::uint8_t)
DEFINE_FIELD_UINT(std::uint16_t)
DEFINE_FIELD_UINT(std::uint32_t)
DEFINE_FIELD_UINT(std::uint64_t)

#undef DEFINE_FIELD_INT
#undef DEFINE_FIELD_UINT

InfluxMeasurementStreamWriter& InfluxMeasurementStreamWriter::withField(const char* key, float value)
{
    beginField(key);
    char buf[32];
    int len = std::snprintf(buf, sizeof(buf), "%g", static_cast<double>(value));
    _stream.write(buf, static_cast<std::size_t>(len));
    return *this;
}

InfluxMeasurementStreamWriter& InfluxMeasurementStreamWriter::withField(const char* key, double value)
{
    beginField(key);
    char buf[32];
    int len = std::snprintf(buf, sizeof(buf), "%g", value);
    _stream.write(buf, static_cast<std::size_t>(len));
    return *this;
}

InfluxMeasurementStreamWriter& InfluxMeasurementStreamWriter::withField(const char* key, bool value)
{
    beginField(key);
    const char* text = value ? "true" : "false";
    _stream.write(text, std::strlen(text));
    return *this;
}

// ========== MEASUREMENT CONTROL ==========

InfluxMeasurementStreamWriter& InfluxMeasurementStreamWriter::withMeasurement(const char* name, std::int64_t timestamp)
{
    char buf[32];
    int len = std::snprintf(buf, sizeof(buf), " %lld\n", static_cast<long long>(_currentTimestamp));
    _stream.write(buf, static_cast<std::size_t>(len));
    beginNewMeasurement(name, timestamp);
    return *this;
}

void InfluxMeasurementStreamWriter::beginNewMeasurement(const char* name, std::int64_t timestamp)
{
    _writer.writeEscapedKey(name);
    _currentName = name;
    _currentTimestamp = timestamp;
    _hasFields = false;
    _hasTags = false;
}

void InfluxMeasurementStreamWriter::Finish()
{
    char buf[32];
    int len = std::snprintf(buf, sizeof(buf), " %lld\n", static_cast<long long>(_currentTimestamp));
    _stream.write(buf, static_cast<std::size_t>(len));
    _stream.flush();
    _client.endRequest();
}
