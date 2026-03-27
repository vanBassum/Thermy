#pragma once

#include <cstdint>
#include <cstddef>
#include <cstring>

/// Interface for flash memory hardware.
class IFlash {
public:
    virtual ~IFlash() = default;

    /// Properties
    virtual size_t sectorSize() const = 0;
    virtual size_t sectorCount() const = 0;
    virtual size_t totalSize() const = 0;

    /// Write data to flash. On real flash, can only clear bits (1->0).
    /// Returns number of bytes successfully written.
    virtual size_t write(size_t address, const uint8_t* data, size_t length) = 0;

    /// Read data from flash.
    /// Returns number of bytes successfully read.
    virtual size_t read(size_t address, uint8_t* data, size_t length) const = 0;

    /// Erase a full sector (sets all bytes in sector to 0xFF).
    virtual bool eraseSector(size_t sectorIndex) = 0;
};

#if defined(__GNUC__) || defined(__clang__)
#define FLASH_LOG_PACKED __attribute__((packed))
#else
#define FLASH_LOG_PACKED
#endif

struct FLASH_LOG_PACKED FlashLogHeader {
    uint32_t magic;
    uint32_t version;
    uint32_t keySize;
    uint32_t valueSize;
    uint32_t crc;
};

static_assert(sizeof(FlashLogHeader) == 20, "FlashLogHeader must be 20 bytes, no padding");

/// On-flash flags byte that precedes every key-value segment (see DESIGN.md).
///
/// NOR-flash semantics: erased = 0xFF (all 1s), writing can only clear bits (1->0).
///   0xFF = erased     (no segment written here)
///   0xBF = continuation segment  (bit 6 cleared)
///   0x3F = first segment of entry (bits 6+7 cleared)
///   0x00 = tombstone  (all bits cleared)
struct FLASH_LOG_PACKED SegmentFlags {
    uint8_t reserved     : 6;  // bits 0-5, kept as 1 on write
    uint8_t segment      : 1;  // bit 6, cleared on all written segments
    uint8_t firstSegment : 1;  // bit 7, cleared on first segment of entry

    bool isFirst() const        { return firstSegment == 0 && segment == 0 && reserved != 0; }
    bool isContinuation() const { return firstSegment == 1 && segment == 0; }
    bool isErased() const       { return toByte() == 0xFF; }
    bool isTombstone() const    { return toByte() == 0x00; }
    bool isWritten() const      { return segment == 0; }
    uint8_t segmentCount() const { return reserved; }

    uint8_t toByte() const {
        uint8_t byte;
        std::memcpy(&byte, this, 1);
        return byte;
    }

    static SegmentFlags fromByte(uint8_t byte) {
        SegmentFlags flags;
        std::memcpy(&flags, &byte, 1);
        return flags;
    }
};

static_assert(sizeof(SegmentFlags) == 1, "SegmentFlags must be 1 byte");

class FlashLog;

class EntryIterator {
public:
    EntryIterator(const FlashLog& log);

    bool operator==(const EntryIterator& other) const;
    bool operator!=(const EntryIterator& other) const;
    EntryIterator& operator++();
    const EntryIterator& operator*() const { return *this; }

    uint32_t fieldCount() const;
    bool valid() const;
    bool atEnd() const { return offset == END_OFFSET; }

    bool readKey(uint32_t fieldIndex, void* key) const;
    bool readValue(uint32_t fieldIndex, void* value) const;
    size_t readData(uint32_t fieldIndex, void* buffer, size_t maxLength) const;

    template<typename K>
    K key(uint32_t fieldIndex) const {
        K k{};
        readKey(fieldIndex, &k);
        return k;
    }

    template<typename V>
    V value(uint32_t fieldIndex) const {
        V v{};
        readValue(fieldIndex, &v);
        return v;
    }

private:
    friend class FlashLog;
    static constexpr size_t END_OFFSET = SIZE_MAX;

    EntryIterator(const FlashLog& log, size_t offset);

    void scanToNextEntry();
    uint32_t computeEntryCrc() const;

    const FlashLog& log;
    size_t offset;
    uint32_t fields;
    uint32_t segmentSize;
    uint32_t entryCrc;
};

/// Logs structured entries sequentially to flash memory.
class FlashLog {
    friend class EntryIterator;
public:
    explicit FlashLog(IFlash& flashDriver);

    bool init();
    bool format(size_t keySize, size_t valueSize);

    const FlashLogHeader& header() const;

    bool beginEntry();
    bool field(const void* key, const void* data, size_t dataLength);
    bool finishEntry();
    uint32_t entryCount() const;

    EntryIterator begin() const;
    EntryIterator end() const;

    bool updateValue(const EntryIterator& entry, uint32_t fieldIndex, const void* value);
    bool updateValue(const EntryIterator& entry, uint32_t fieldIndex, const void* data, size_t dataLength);

    template<typename V>
    bool updateValue(const EntryIterator& entry, uint32_t fieldIndex, const V& value) {
        return updateValue(entry, fieldIndex, static_cast<const void*>(&value), sizeof(V));
    }

    template<typename K, typename V>
    bool field(const K& key, const V& value) {
        return field(static_cast<const void*>(&key),
                     static_cast<const void*>(&value),
                     sizeof(V));
    }

private:
    size_t dataStartOffset() const;
    size_t adjustForHeaders(size_t offset, size_t segSize) const;
    void eraseSectorSafe(size_t sectorIndex);

    IFlash& flashDevice;
    FlashLogHeader storedHeader;
    bool building;
    uint32_t storedEntryCount;
    size_t writeOffset;
    size_t entryStartOffset;
    uint32_t currentFieldCount;
};
