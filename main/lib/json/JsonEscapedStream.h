#pragma once
#include "Stream.h"
#include <cstdint>
#include <cstdio>
#include <cassert>

class JsonEscapedStream : public Stream
{
    Stream &out;

    static void writeHex(Stream &s, uint8_t c)
    {
        char buf[7]; // 6 characters + null terminator
        int n = snprintf(buf, sizeof(buf), "\\u%04X", c);
        if (n > 0 && n < static_cast<int>(sizeof(buf)))
            s.write(buf, n); // write only the characters, not the terminator
    }

public:
    explicit JsonEscapedStream(Stream &s) : out(s) {}

    size_t write(const void *data, size_t len) override
    {
        const char *p = static_cast<const char *>(data);
        size_t written = 0;

        for (size_t i = 0; i < len; ++i)
        {
            char c = p[i];
            switch (c)
            {
            case '\"':
                out.write("\\\"", 2);
                break;
            case '\\':
                out.write("\\\\", 2);
                break;
            case '\b':
                out.write("\\b", 2);
                break;
            case '\f':
                out.write("\\f", 2);
                break;
            case '\n':
                out.write("\\n", 2);
                break;
            case '\r':
                out.write("\\r", 2);
                break;
            case '\t':
                out.write("\\t", 2);
                break;
            default:
                if (static_cast<unsigned char>(c) < 0x20)
                {
                    writeHex(out, static_cast<uint8_t>(c));
                }
                else
                {
                    out.write(&c, 1);
                }
                break;
            }
            ++written;
        }
        return written;
    }

    size_t read(void *, size_t) override
    {
        assert(false && "JsonEscapedStream does not support read()");
        return 0;
    }

    void flush() override
    {
        out.flush();
    }
};
