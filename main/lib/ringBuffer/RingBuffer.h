#pragma once
#include "rtos.h"
#include "HashUtils.h"
#include <string.h>
#include <assert.h>

template<typename T, size_t MAX_ENTRIES>
class RingBuffer
{
    inline constexpr static const char* TAG = "RingBuffer";
    static_assert(std::is_trivially_copyable_v<T>,
              "RingBuffer<T> requires T to be trivially copyable (no pointers, virtual methods, or dynamic memory)");


public:
    RingBuffer() = default;

    // ---- Nested Iterator ----
    class Iterator
    {
    public:
        explicit Iterator(RingBuffer& manager, size_t index, uint32_t hash)
            : _manager(manager), _index(index), _hash(hash) {}

        bool IsValid()
        {
            return _hash == _manager.GetEntryHash(_index);
        }

        bool Read(T& out)
        {
            uint32_t hashNow = 0;
            if (!_manager.Read(_index, out, hashNow))
                return false;
            return hashNow == _hash;
        }

        bool Advance()
        {
            return _manager.AdvanceIndex(_index, _hash);
        }

        bool ReadAndAdvance(T& out)
        {
            if (!Read(out))
                return false;
            Advance(); // even if fails, read was OK
            return true;
        }

        bool Write(const T& value)
        {
            return _manager.Write(_index, value, _hash);
        }

    private:
        RingBuffer& _manager;
        size_t _index = 0;
        uint32_t _hash = 0;
    };

    // ---- Common API ----
    Iterator GetIterator()
    {
        LOCK(mutex);
        size_t startIndex = readPtr;
        uint32_t hash = HashUtils::FastHash(reinterpret_cast<uint8_t*>(&entries[startIndex]), sizeof(T));
        return Iterator(*this, startIndex, hash);
    }

    void Append(const T& entry)
    {
        LOCK(mutex);
        memcpy(&entries[writePtr], &entry, sizeof(T));
        writePtr = (writePtr + 1) % MAX_ENTRIES;
        if (writePtr == readPtr)
            readPtr = (readPtr + 1) % MAX_ENTRIES; // overwrite oldest
    }

protected:
    bool Read(size_t index, T& out, uint32_t& outHash)
    {
        LOCK(mutex);
        if (index >= MAX_ENTRIES)
            return false;
        memcpy(&out, &entries[index], sizeof(T));
        outHash = HashUtils::FastHash(reinterpret_cast<const uint8_t*>(&entries[index]), sizeof(T));
        return true;
    }

    bool Write(size_t index, const T& value, uint32_t& hash)
    {
        LOCK(mutex);
        if (index >= MAX_ENTRIES)
            return false;

        uint32_t currentHash = HashUtils::FastHash(reinterpret_cast<const uint8_t*>(&entries[index]), sizeof(T));
        if (hash != currentHash)
            return false; // modified/overwritten

        // ---- Flash semantics: only allow 1→0 transitions ----
        const uint8_t* newData = reinterpret_cast<const uint8_t*>(&value);
        uint8_t* oldData = reinterpret_cast<uint8_t*>(&entries[index]);
        for (size_t i = 0; i < sizeof(T); ++i)
        {
            assert((oldData[i] & newData[i]) == newData[i] && "Invalid write: cannot set bits from 0→1 (flash semantics)");
        }

        memcpy(&entries[index], &value, sizeof(T));
        hash = HashUtils::FastHash(reinterpret_cast<const uint8_t*>(&entries[index]), sizeof(T));
        return true;
    }

    bool AdvanceIndex(size_t& index, uint32_t& hash)
    {
        LOCK(mutex);
        if (hash != HashUtils::FastHash(reinterpret_cast<const uint8_t*>(&entries[index]), sizeof(T)))
            return false;

        size_t next = (index + 1) % MAX_ENTRIES;
        if (next == writePtr)
            return false; // end reached

        index = next;
        hash = HashUtils::FastHash(reinterpret_cast<const uint8_t*>(&entries[index]), sizeof(T));
        return true;
    }

    uint32_t GetEntryHash(size_t index)
    {
        LOCK(mutex);
        if (index >= MAX_ENTRIES)
            return 0;
        return HashUtils::FastHash(reinterpret_cast<const uint8_t*>(&entries[index]), sizeof(T));
    }

private:
    Mutex mutex;
    T entries[MAX_ENTRIES] = {};  // fully typed storage
    size_t writePtr = 0;
    size_t readPtr = 0;
};
