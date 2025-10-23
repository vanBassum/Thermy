#pragma once
#include <cstdint>
#include <cstdio>
#include <cstring>
#include "Stream.h"
#include "JsonEscapedStream.h"

class JsonStreamWriter
{
    Stream &out;

public:
    explicit JsonStreamWriter(Stream &s) : out(s) {}

    void writeBool(bool v)
    {
        const char *str = v ? "true" : "false";
        out.write(str, std::strlen(str));
    }

    void writeInt(int64_t v)
    {
        char buf[32];
        int n = snprintf(buf, sizeof(buf), "%lld",
                         static_cast<long long>(v));
        out.write(buf, n);
    }

    void writeUInt(uint64_t v)
    {
        char buf[32];
        int n = snprintf(buf, sizeof(buf), "%llu",
                         static_cast<unsigned long long>(v));
        out.write(buf, n);
    }

    void writeFloat(float v)
    {
        char buf[32];
        int n = snprintf(buf, sizeof(buf), "%g",
                         static_cast<double>(v));
        out.write(buf, n);
    }

    void writeDouble(double v)
    {
        char buf[64];
        int n = snprintf(buf, sizeof(buf), "%g", v);
        out.write(buf, n);
    }

    void writeString(const char *v)
    {
        out.write("\"", 1);
        JsonEscapedStream esc(out);
        esc.write(v, std::strlen(v));
        esc.flush();
        out.write("\"", 1);
    }

    inline constexpr static const char b64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    void writeData(const void *data, size_t len)
    {
        const uint8_t *bytes = static_cast<const uint8_t *>(data);
        out.write("\"", 1);
        size_t i = 0;
        while (i + 2 < len)
        {
            uint32_t triple = (bytes[i] << 16) | (bytes[i + 1] << 8) | bytes[i + 2];
            char buf[4] = {
                b64_table[(triple >> 18) & 0x3F],
                b64_table[(triple >> 12) & 0x3F],
                b64_table[(triple >> 6) & 0x3F],
                b64_table[triple & 0x3F]};
            out.write(buf, 4);
            i += 3;
        }

        // Handle remaining bytes
        if (i < len)
        {
            uint32_t triple = bytes[i] << 16;
            char buf[4];
            if (i + 1 < len)
            {
                triple |= bytes[i + 1] << 8;
                buf[0] = b64_table[(triple >> 18) & 0x3F];
                buf[1] = b64_table[(triple >> 12) & 0x3F];
                buf[2] = b64_table[(triple >> 6) & 0x3F];
                buf[3] = '=';
                out.write(buf, 4);
            }
            else
            {
                buf[0] = b64_table[(triple >> 18) & 0x3F];
                buf[1] = b64_table[(triple >> 12) & 0x3F];
                buf[2] = '=';
                buf[3] = '=';
                out.write(buf, 4);
            }
        }

        out.write("\"", 1);
    }
};
