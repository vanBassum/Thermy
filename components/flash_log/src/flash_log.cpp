#include "flash_log.h"
#include <cstring>

static constexpr uint32_t MAGIC = 0x464C4F47; // "FLOG"
static constexpr uint32_t VERSION = 1;

static uint32_t crc32(const uint8_t* data, size_t length) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < length; ++i) {
        crc ^= data[i];
        for (int bit = 0; bit < 8; ++bit) {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xEDB88320;
            else
                crc >>= 1;
        }
    }
    return ~crc;
}

static bool readAndValidateHeader(const IFlash& flash, size_t offset, FlashLogHeader& out) {
    if (flash.read(offset, reinterpret_cast<uint8_t*>(&out), sizeof(out)) != sizeof(out)) {
        return false;
    }
    if (out.magic != MAGIC) {
        return false;
    }
    size_t dataLength = offsetof(FlashLogHeader, crc);
    return crc32(reinterpret_cast<const uint8_t*>(&out), dataLength) == out.crc;
}

FlashLog::FlashLog(IFlash& flashDriver)
    : flashDevice(flashDriver)
    , storedHeader{}
    , building(false)
    , storedEntryCount(0)
    , writeOffset(0)
    , entryStartOffset(0)
    , currentFieldCount(0)
{
}

bool FlashLog::init() {
    FlashLogHeader readHeader{};

    // Try primary header (sector 0), fall back to backup (sector 1)
    if (!readAndValidateHeader(flashDevice, 0, readHeader) &&
        !readAndValidateHeader(flashDevice, flashDevice.sectorSize(), readHeader)) {
        return false;
    }

    storedHeader = readHeader;

    // Scan flash to find write position and entry count.
    // writeOffset = first erased slot (the free space gap).
    // Continue scanning past the gap for entryCount only.
    size_t segSize = 1 + storedHeader.keySize + storedHeader.valueSize;
    size_t scanOffset = dataStartOffset();
    writeOffset = scanOffset;
    storedEntryCount = 0;
    bool foundFreeSpace = false;

    while (scanOffset + segSize <= flashDevice.totalSize()) {
        scanOffset = adjustForHeaders(scanOffset, segSize);
        if (scanOffset + segSize > flashDevice.totalSize()) break;

        uint8_t flagsByte;
        flashDevice.read(scanOffset, &flagsByte, 1);
        SegmentFlags flags = SegmentFlags::fromByte(flagsByte);

        if (flags.isFirst()) {
            storedEntryCount++;
            scanOffset += flags.segmentCount() * segSize;
            if (!foundFreeSpace) writeOffset = scanOffset;
        } else if (flags.isErased()) {
            if (!foundFreeSpace) {
                writeOffset = scanOffset;
                foundFreeSpace = true;
            }
            scanOffset += segSize;
        } else {
            scanOffset += segSize;
            if (!foundFreeSpace) writeOffset = scanOffset;
        }
    }

    return true;
}

bool FlashLog::format(size_t keySize, size_t valueSize) {
    if (keySize == 0 || valueSize == 0) {
        return false;
    }
    if (flashDevice.sectorCount() < 3 ||
        flashDevice.sectorSize() < sizeof(FlashLogHeader)) {
        return false;
    }

    for (size_t i = 0; i < flashDevice.sectorCount(); ++i) {
        if (!flashDevice.eraseSector(i)) {
            return false;
        }
    }

    FlashLogHeader writeHeader{MAGIC, VERSION, static_cast<uint32_t>(keySize), static_cast<uint32_t>(valueSize), 0};
    size_t dataLength = offsetof(FlashLogHeader, crc);
    writeHeader.crc = crc32(reinterpret_cast<const uint8_t*>(&writeHeader), dataLength);

    // Write primary header to sector 0
    if (flashDevice.write(0, reinterpret_cast<const uint8_t*>(&writeHeader), sizeof(writeHeader)) != sizeof(writeHeader)) {
        return false;
    }

    // Write backup header to sector 1
    size_t backupOffset = flashDevice.sectorSize();
    return flashDevice.write(backupOffset, reinterpret_cast<const uint8_t*>(&writeHeader), sizeof(writeHeader)) == sizeof(writeHeader);
}

size_t FlashLog::dataStartOffset() const {
    return sizeof(FlashLogHeader);
}

size_t FlashLog::adjustForHeaders(size_t offset, size_t segSize) const {
    size_t backupStart = flashDevice.sectorSize();
    size_t backupEnd = backupStart + sizeof(FlashLogHeader);
    if (offset + segSize > backupStart && offset < backupEnd) {
        return backupEnd;
    }
    return offset;
}

void FlashLog::eraseSectorSafe(size_t sectorIndex) {
    if (sectorIndex <= 1) {
        FlashLogHeader hdr{};
        size_t hdrOffset = sectorIndex * flashDevice.sectorSize();
        flashDevice.read(hdrOffset, reinterpret_cast<uint8_t*>(&hdr), sizeof(hdr));
        flashDevice.eraseSector(sectorIndex);
        flashDevice.write(hdrOffset, reinterpret_cast<const uint8_t*>(&hdr), sizeof(hdr));
    } else {
        flashDevice.eraseSector(sectorIndex);
    }
}

const FlashLogHeader& FlashLog::header() const {
    return storedHeader;
}

bool FlashLog::beginEntry() {
    if (building) {
        return false;
    }
    building = true;
    entryStartOffset = writeOffset;
    currentFieldCount = 0;
    return true;
}

static constexpr uint32_t MAX_FIELDS_PER_ENTRY = 63;

bool FlashLog::field(const void* key, const void* data, size_t dataLength) {
    if (!building) {
        return false;
    }

    size_t segmentSize = 1 + storedHeader.keySize + storedHeader.valueSize;
    uint32_t segmentsNeeded = static_cast<uint32_t>(
        (dataLength + storedHeader.valueSize - 1) / storedHeader.valueSize);

    if (segmentsNeeded == 0) segmentsNeeded = 1;

    if (currentFieldCount + segmentsNeeded > MAX_FIELDS_PER_ENTRY) {
        return false;
    }

    // Reject duplicate keys within the same entry (check once before writing)
    for (uint32_t i = 0; i < currentFieldCount; i++) {
        size_t existingKeyAddr = entryStartOffset + i * segmentSize + 1;
        bool match = true;
        for (uint32_t b = 0; b < storedHeader.keySize; b++) {
            uint8_t existing = 0;
            flashDevice.read(existingKeyAddr + b, &existing, 1);
            if (existing != static_cast<const uint8_t*>(key)[b]) {
                match = false;
                break;
            }
        }
        if (match) {
            return false;
        }
    }

    // Write segments (one per valueSize chunk)
    const uint8_t* dataPtr = static_cast<const uint8_t*>(data);
    size_t remaining = dataLength;

    for (uint32_t s = 0; s < segmentsNeeded; s++) {
        // Circular buffer: wrap when we reach the end of flash
        if (writeOffset + segmentSize > flashDevice.totalSize()) {
            if (currentFieldCount > 0) {
                return false;  // can't wrap mid-entry
            }
            writeOffset = dataStartOffset();
            entryStartOffset = writeOffset;
        }

        // Skip over backup header region if segment would overlap it
        size_t adjusted = adjustForHeaders(writeOffset, segmentSize);
        if (adjusted != writeOffset) {
            if (currentFieldCount > 0) {
                return false;  // can't jump over header mid-entry
            }
            writeOffset = adjusted;
            entryStartOffset = writeOffset;
        }

        // Erase sector if the slot has old data
        uint8_t probe = 0xFF;
        flashDevice.read(writeOffset, &probe, 1);
        if (probe != 0xFF) {
            eraseSectorSafe(writeOffset / flashDevice.sectorSize());
        }

        // Erase next sector if segment spans a boundary
        size_t endOffset = writeOffset + segmentSize - 1;
        if (endOffset / flashDevice.sectorSize() != writeOffset / flashDevice.sectorSize()) {
            flashDevice.read(endOffset, &probe, 1);
            if (probe != 0xFF) {
                eraseSectorSafe(endOffset / flashDevice.sectorSize());
            }
        }

        // Write flags
        uint8_t flags = 0xBF;
        flashDevice.write(writeOffset, &flags, 1);

        // Write key
        flashDevice.write(writeOffset + 1,
                          static_cast<const uint8_t*>(key),
                          storedHeader.keySize);

        // Write value chunk (pad with 0 if last chunk is short)
        size_t chunkSize = remaining < storedHeader.valueSize ? remaining : storedHeader.valueSize;
        flashDevice.write(writeOffset + 1 + storedHeader.keySize, dataPtr, chunkSize);

        dataPtr += chunkSize;
        remaining -= chunkSize;
        writeOffset += segmentSize;
        currentFieldCount++;
    }

    return true;
}

bool FlashLog::updateValue(const EntryIterator& entry, uint32_t fieldIndex, const void* value) {
    return updateValue(entry, fieldIndex, value, storedHeader.valueSize);
}

bool FlashLog::updateValue(const EntryIterator& entry, uint32_t fieldIndex,
                           const void* data, size_t dataLength) {
    if (entry.atEnd() || fieldIndex >= entry.fields) {
        return false;
    }

    size_t segSize = 1 + storedHeader.keySize + storedHeader.valueSize;
    uint32_t segmentsNeeded = static_cast<uint32_t>(
        (dataLength + storedHeader.valueSize - 1) / storedHeader.valueSize);

    if (fieldIndex + segmentsNeeded > entry.fields) {
        return false;
    }

    const uint8_t* dataPtr = static_cast<const uint8_t*>(data);
    size_t remaining = dataLength;

    for (uint32_t s = 0; s < segmentsNeeded; s++) {
        size_t addr = entry.offset + (fieldIndex + s) * segSize + 1 + storedHeader.keySize;
        size_t chunkSize = remaining < storedHeader.valueSize ? remaining : storedHeader.valueSize;

        // NOR flash write() only clears bits — hardware enforces the constraint
        flashDevice.write(addr, dataPtr, chunkSize);

        dataPtr += chunkSize;
        remaining -= chunkSize;
    }

    return true;
}

bool FlashLog::finishEntry() {
    if (!building) {
        return false;
    }
    building = false;

    // No fields written — nothing to commit
    if (writeOffset == entryStartOffset) {
        return true;
    }

    // Promote first segment and encode field count in reserved bits (0-5).
    // Byte value = fieldCount & 0x3F (bits 7,6 cleared = first segment).
    // NOR-safe: 0xBF & N clears bits 7,6 and unused reserved bits.
    uint8_t flags = static_cast<uint8_t>(currentFieldCount & 0x3F);
    flashDevice.write(entryStartOffset, &flags, 1);

    storedEntryCount++;
    return true;
}

uint32_t FlashLog::entryCount() const {
    return storedEntryCount;
}

EntryIterator FlashLog::begin() const {
    return EntryIterator(*this);
}

EntryIterator FlashLog::end() const {
    return EntryIterator(*this, EntryIterator::END_OFFSET);
}

// --- EntryIterator ---

EntryIterator::EntryIterator(const FlashLog& log)
    : log(log)
    , offset(log.dataStartOffset())
    , fields(0)
    , segmentSize(1 + log.storedHeader.keySize + log.storedHeader.valueSize)
    , entryCrc(0)
{
    scanToNextEntry();
}

EntryIterator::EntryIterator(const FlashLog& log, size_t offset)
    : log(log)
    , offset(offset)
    , fields(0)
    , segmentSize(1 + log.storedHeader.keySize + log.storedHeader.valueSize)
    , entryCrc(0)
{
}

void EntryIterator::scanToNextEntry() {
    size_t flashSize = log.flashDevice.totalSize();
    while (offset + segmentSize <= flashSize) {
        // Skip backup header region
        offset = log.adjustForHeaders(offset, segmentSize);

        if (offset + segmentSize > flashSize) break;

        uint8_t flagsByte = 0xFF;
        log.flashDevice.read(offset, &flagsByte, 1);
        SegmentFlags flags = SegmentFlags::fromByte(flagsByte);

        if (flags.isFirst()) {
            fields = flags.segmentCount();
            entryCrc = computeEntryCrc();
            return;
        }

        offset += segmentSize;
    }

    offset = END_OFFSET;
}

bool EntryIterator::operator==(const EntryIterator& other) const {
    return offset == other.offset;
}

bool EntryIterator::operator!=(const EntryIterator& other) const {
    return offset != other.offset;
}

EntryIterator& EntryIterator::operator++() {
    offset += fields * segmentSize;
    scanToNextEntry();
    return *this;
}

uint32_t EntryIterator::fieldCount() const {
    return fields;
}

uint32_t EntryIterator::computeEntryCrc() const {
    if (atEnd() || fields == 0) return 0;

    size_t entryBytes = fields * segmentSize;
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < entryBytes; i++) {
        uint8_t byte = 0xFF;
        log.flashDevice.read(offset + i, &byte, 1);
        crc ^= byte;
        for (int bit = 0; bit < 8; bit++) {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xEDB88320;
            else
                crc >>= 1;
        }
    }
    return ~crc;
}

bool EntryIterator::valid() const {
    if (atEnd()) return false;
    return computeEntryCrc() == entryCrc;
}

bool EntryIterator::readKey(uint32_t fieldIndex, void* key) const {
    if (atEnd() || fieldIndex >= fields) return false;
    size_t addr = offset + fieldIndex * segmentSize + 1;
    uint32_t keySize = log.storedHeader.keySize;
    return log.flashDevice.read(addr, static_cast<uint8_t*>(key), keySize) == keySize;
}

bool EntryIterator::readValue(uint32_t fieldIndex, void* value) const {
    if (atEnd() || fieldIndex >= fields) return false;
    uint32_t keySize = log.storedHeader.keySize;
    uint32_t valueSize = log.storedHeader.valueSize;
    size_t addr = offset + fieldIndex * segmentSize + 1 + keySize;
    return log.flashDevice.read(addr, static_cast<uint8_t*>(value), valueSize) == valueSize;
}

size_t EntryIterator::readData(uint32_t fieldIndex, void* buffer, size_t maxLength) const {
    if (atEnd() || fieldIndex >= fields) return 0;

    uint32_t keySize = log.storedHeader.keySize;
    uint32_t valueSize = log.storedHeader.valueSize;

    // Read the reference key at fieldIndex
    uint8_t refKey[64];
    if (!readKey(fieldIndex, refKey)) return 0;

    uint8_t* dst = static_cast<uint8_t*>(buffer);
    size_t bytesRead = 0;

    for (uint32_t i = fieldIndex; i < fields && bytesRead < maxLength; i++) {
        // Stop when key changes
        uint8_t currentKey[64];
        if (!readKey(i, currentKey)) break;
        if (std::memcmp(refKey, currentKey, keySize) != 0) break;

        size_t toRead = maxLength - bytesRead;
        if (toRead > valueSize) toRead = valueSize;

        size_t addr = offset + i * segmentSize + 1 + keySize;
        log.flashDevice.read(addr, dst + bytesRead, toRead);
        bytesRead += toRead;
    }

    return bytesRead;
}
