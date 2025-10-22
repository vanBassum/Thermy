#include "StringWriter.h"
#include <cstdio>
#include <cstdarg>


StringWriter::StringWriter(Stream &stream)
    : _stream(stream), _converter(StringConverter::Default()) {}

StringWriter::StringWriter(Stream& stream, const StringConverter& converter)
    : _stream(stream), _converter(converter) {}

// --- Implementations ---
void StringWriter::writeChar(char c) {
    _stream.write(&c, 1);
}

void StringWriter::writeString(const char* s) {
    if (!s) return;
    writeBufferInChunks(s, std::strlen(s));
}

void StringWriter::writeString(const char *s, size_t len)
{
    if (!s) return;
    writeBufferInChunks(s, len);
}

// --- Type writers ---
void StringWriter::writeInt8(int8_t value)       { writeUsingConverter(value, &StringConverter::Int8ToString); }
void StringWriter::writeUInt8(uint8_t value)     { writeUsingConverter(value, &StringConverter::UInt8ToString); }
void StringWriter::writeInt16(int16_t value)     { writeUsingConverter(value, &StringConverter::Int16ToString); }
void StringWriter::writeUInt16(uint16_t value)   { writeUsingConverter(value, &StringConverter::UInt16ToString); }
void StringWriter::writeInt32(int32_t value)     { writeUsingConverter(value, &StringConverter::Int32ToString); }
void StringWriter::writeUInt32(uint32_t value)   { writeUsingConverter(value, &StringConverter::UInt32ToString); }
void StringWriter::writeInt64(int64_t value)     { writeUsingConverter(value, &StringConverter::Int64ToString); }
void StringWriter::writeUInt64(uint64_t value)   { writeUsingConverter(value, &StringConverter::UInt64ToString); }
void StringWriter::writeBool(bool value)         { writeUsingConverter(value, &StringConverter::BoolToString); }
void StringWriter::writeFloat(float value)       { writeUsingConverter(value, &StringConverter::FloatToString); }

void StringWriter::writeBlob(const void* data, size_t len) {
    if (!data || len == 0)
        return;

    const uint8_t* bytes = static_cast<const uint8_t*>(data);
    char buf[256];
    const size_t bytesPerChunk = (sizeof(buf) - 1) / 2;

    for (size_t i = 0; i < len; i += bytesPerChunk) {
        size_t chunkLen = (i + bytesPerChunk <= len) ? bytesPerChunk : (len - i);
        if (_converter.BlobToString(buf, sizeof(buf), bytes + i, chunkLen))
            writeBufferInChunks(buf, std::strlen(buf));
    }
}

void StringWriter::writeBufferInChunks(const char* buffer, size_t length) {
    constexpr size_t CHUNK_SIZE = 128;
    for (size_t offset = 0; offset < length; offset += CHUNK_SIZE) {
        size_t chunkLen = (offset + CHUNK_SIZE <= length)
                            ? CHUNK_SIZE
                            : (length - offset);
        _stream.write(buffer + offset, chunkLen);
    }
}
