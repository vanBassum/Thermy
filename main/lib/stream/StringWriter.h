#pragma once
#include "Stream.h"
#include <cstdarg>
#include <cstdio>
#include <cstdint>
#include <cstring>

class StringWriter {
public:
    explicit StringWriter(Stream& stream) : _stream(stream) {}

    void writeChar(char c) {
        _stream.write(&c, 1);
    }

    void writeString(const char* s) {
        if (s) _stream.write(s, std::strlen(s));
    }

    void writeFormat(const char* fmt, ...) {
        char buf[64];
        va_list args;
        va_start(args, fmt);
        int len = std::vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        if (len > 0) {
            if (len > static_cast<int>(sizeof(buf))) len = sizeof(buf);
            _stream.write(buf, len);
        }
    }

    void writeInt64(int64_t value) {
        char buf[32];
        int len = std::snprintf(buf, sizeof(buf), "%lld", static_cast<long long>(value));
        _stream.write(buf, len);
    }

    void writeUInt64(uint64_t value) {
        char buf[32];
        int len = std::snprintf(buf, sizeof(buf), "%llu", static_cast<unsigned long long>(value));
        _stream.write(buf, len);
    }

    void writeFloat(float value, int precision = 6) {
        char fmt[8];
        std::snprintf(fmt, sizeof(fmt), "%%.%df", precision);
        char buf[32];
        int len = std::snprintf(buf, sizeof(buf), fmt, value);
        _stream.write(buf, len);
    }

private:
    Stream& _stream;
};
