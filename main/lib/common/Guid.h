#pragma once
#include <cstdint>
#include <cstring>
#include <esp_system.h>

class GUID
{
public:
    uint8_t raw[16];

    GUID() 
	{ 
		memset(raw, 0, 16); 
	} 

    static GUID NewRandom()
    {
        GUID guid;
        esp_fill_random(guid.raw, 16);
        return guid;
    }

    static GUID Zero()
    {
        return GUID(); 
    }

    void ToString(char* buffer, size_t size) const
    {
        if (size < 33) // 32 chars + null terminator
        {
            if (size > 0)
                buffer[0] = 0;
            return;
        }

        for (int i = 0; i < 16; i++)
        {
            sprintf(&buffer[i * 2], "%02X", raw[i]);
        }
        buffer[32] = '\0';
    }

    bool operator==(const GUID& other) const { return memcmp(raw, other.raw, 16) == 0; }
    bool operator!=(const GUID& other) const { return memcmp(raw, other.raw, 16) != 0; }

private:
    // Private ctor for raw bytes (factories only)
    explicit GUID(const uint8_t* bytes)
    {
        memcpy(raw, bytes, 16);
    }
};
