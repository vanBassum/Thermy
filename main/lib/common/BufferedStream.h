#pragma once
#include "Stream.h"
#include <cstring>
#include <algorithm>

// Templated buffered stream
template <size_t BufferSize>
class BufferedStream : public Stream
{
public:
    explicit BufferedStream(Stream& underlying)
        : _underlying(underlying), _writePos(0) {}

    ~BufferedStream() override
    {
        flush(); // Ensure all data is written
    }

    size_t write(const void* data, size_t len) override
    {
        const uint8_t* src = static_cast<const uint8_t*>(data);
        size_t totalWritten = 0;

        while (len > 0)
        {
            size_t spaceLeft = BufferSize - _writePos;
            size_t toCopy = std::min(len, spaceLeft);

            memcpy(_buffer + _writePos, src, toCopy);
            _writePos += toCopy;
            src += toCopy;
            len -= toCopy;
            totalWritten += toCopy;

            if (_writePos >= BufferSize)
                flush();
        }

        return totalWritten;
    }

    size_t read(void* buffer, size_t len) override
    {
        // No read buffering (pass-through)
        return _underlying.read(buffer, len);
    }

    void flush() override
    {
        if (_writePos > 0)
        {
            _underlying.write(_buffer, _writePos);
            _underlying.flush();
            _writePos = 0;
        }
    }

private:
    Stream& _underlying;
    uint8_t _buffer[BufferSize];
    size_t _writePos;
};
