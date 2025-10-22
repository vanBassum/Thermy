#pragma once
#include "Stream.h"
#include "StringConverter.h"
#include <cstdarg>
#include <cstdint>
#include <cstring>

class StringWriter {
public:
    explicit StringWriter(Stream& stream);
    explicit StringWriter(Stream& stream, const StringConverter& converter);

    void writeChar(char c);
    void writeString(const char* s);
    void writeFormat(const char* fmt, ...);

    void writeInt8(int8_t value);
    void writeUInt8(uint8_t value);
    void writeInt16(int16_t value);
    void writeUInt16(uint16_t value);
    void writeInt32(int32_t value);
    void writeUInt32(uint32_t value);
    void writeInt64(int64_t value);
    void writeUInt64(uint64_t value);
    void writeBool(bool value);
    void writeFloat(float value);
    void writeBlob(const void* data, size_t len);

private:
    template <typename T, typename Method>
    void writeUsingConverter(T value, Method method) {
        char buf[64];
        if ((_converter.*method)(buf, sizeof(buf), value))
            writeBufferInChunks(buf, std::strlen(buf));
    }

    void writeBufferInChunks(const char* buffer, size_t length);

private:
    Stream& _stream;
    const StringConverter& _converter;
};
