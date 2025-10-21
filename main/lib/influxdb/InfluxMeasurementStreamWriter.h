#pragma once
#include <cstdint>
#include "InfluxStreamWriter.h"
#include "RequestStream.h"

class InfluxClient;

// Handles writing one or more measurements to an HTTP stream.
class InfluxMeasurementStreamWriter {
public:
    InfluxMeasurementStreamWriter(RequestStream& stream,
                                  InfluxClient& client,
                                  const char* name,
                                  std::int64_t timestamp);

    // --- Tag writers ---
    InfluxMeasurementStreamWriter& withTag(const char* key, const char* value);
    InfluxMeasurementStreamWriter& withTag(const char* key, std::int8_t value);
    InfluxMeasurementStreamWriter& withTag(const char* key, std::int16_t value);
    InfluxMeasurementStreamWriter& withTag(const char* key, std::int32_t value);
    InfluxMeasurementStreamWriter& withTag(const char* key, std::int64_t value);
    InfluxMeasurementStreamWriter& withTag(const char* key, std::uint8_t value);
    InfluxMeasurementStreamWriter& withTag(const char* key, std::uint16_t value);
    InfluxMeasurementStreamWriter& withTag(const char* key, std::uint32_t value);
    InfluxMeasurementStreamWriter& withTag(const char* key, std::uint64_t value);

    // --- Field writers ---
    InfluxMeasurementStreamWriter& withField(const char* key, const char* value);
    InfluxMeasurementStreamWriter& withField(const char* key, std::int8_t value);
    InfluxMeasurementStreamWriter& withField(const char* key, std::int16_t value);
    InfluxMeasurementStreamWriter& withField(const char* key, std::int32_t value);
    InfluxMeasurementStreamWriter& withField(const char* key, std::int64_t value);
    InfluxMeasurementStreamWriter& withField(const char* key, std::uint8_t value);
    InfluxMeasurementStreamWriter& withField(const char* key, std::uint16_t value);
    InfluxMeasurementStreamWriter& withField(const char* key, std::uint32_t value);
    InfluxMeasurementStreamWriter& withField(const char* key, std::uint64_t value);
    InfluxMeasurementStreamWriter& withField(const char* key, float value);
    InfluxMeasurementStreamWriter& withField(const char* key, double value);
    InfluxMeasurementStreamWriter& withField(const char* key, bool value);

    // --- Start a new measurement ---
    InfluxMeasurementStreamWriter& withMeasurement(const char* name, std::int64_t timestamp);

    // --- Finish writing ---
    void Finish();

private:
    RequestStream& _stream;
    InfluxClient& _client;
    InfluxStreamWriter _writer;
    const char* _currentName;
    std::int64_t _currentTimestamp;
    bool _hasFields;
    bool _hasTags;

    void beginNewMeasurement(const char* name, std::int64_t timestamp);
    void beginField(const char* key);
    void writeIntField(std::int64_t value);
    void writeUIntField(std::uint64_t value);
};
