#pragma once
#include <cstdint>
#include "Stream.h"

// Responsible for writing primitive types and escaping text for Influx line protocol.
class InfluxStreamWriter {
public:
    explicit InfluxStreamWriter(Stream& s) : _stream(s) {}

    // --- Primitive writers ---
    void writeInt8(std::int8_t value);
    void writeInt16(std::int16_t value);
    void writeInt32(std::int32_t value);
    void writeInt64(std::int64_t value);

    void writeUInt8(std::uint8_t value);
    void writeUInt16(std::uint16_t value);
    void writeUInt32(std::uint32_t value);
    void writeUInt64(std::uint64_t value);

    void writeFloat(float value);
    void writeDouble(double value);
    void writeBool(bool value);

    // --- Raw string writer ---
    void writeString(const char* s);

    // --- Escaping helpers (Influx line protocol) ---
    void writeEscapedKey(const char* key);
    void writeEscapedValue(const char* value);

private:
    Stream& _stream;
};
