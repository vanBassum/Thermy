#pragma once
#include <cstdint>
#include "Stream.h"
#include "DateTime.h"

// Writes Influx Line Protocol data to a generic Stream.
class InfluxStreamWriter {
public:
    InfluxStreamWriter(Stream& stream, const char* name, const DateTime& timestamp);

    // --- Tag writers ---
    InfluxStreamWriter& withTag(const char* key, const char* value);
    InfluxStreamWriter& withTag(const char* key, std::int8_t value);
    InfluxStreamWriter& withTag(const char* key, std::int16_t value);
    InfluxStreamWriter& withTag(const char* key, std::int32_t value);
    InfluxStreamWriter& withTag(const char* key, std::int64_t value);
    InfluxStreamWriter& withTag(const char* key, std::uint8_t value);
    InfluxStreamWriter& withTag(const char* key, std::uint16_t value);
    InfluxStreamWriter& withTag(const char* key, std::uint32_t value);
    InfluxStreamWriter& withTag(const char* key, std::uint64_t value);

    // --- Field writers ---
    InfluxStreamWriter& withField(const char* key, const char* value);
    InfluxStreamWriter& withField(const char* key, std::int8_t value);
    InfluxStreamWriter& withField(const char* key, std::int16_t value);
    InfluxStreamWriter& withField(const char* key, std::int32_t value);
    InfluxStreamWriter& withField(const char* key, std::int64_t value);
    InfluxStreamWriter& withField(const char* key, std::uint8_t value);
    InfluxStreamWriter& withField(const char* key, std::uint16_t value);
    InfluxStreamWriter& withField(const char* key, std::uint32_t value);
    InfluxStreamWriter& withField(const char* key, std::uint64_t value);
    InfluxStreamWriter& withField(const char* key, float value);
    InfluxStreamWriter& withField(const char* key, double value);
    InfluxStreamWriter& withField(const char* key, bool value);

    // --- Start a new measurement (same stream) ---
    InfluxStreamWriter& withMeasurement(const char* name, const DateTime& timestamp);

    // --- Finish writing this batch ---
    void Finish();

private:
    Stream& _stream;
    bool _hasTags;
    bool _hasFields;
    DateTime _timestamp;

    void writeEscaped(const char* s);
    void writeKeyValue(const char* key, const char* value);
};
