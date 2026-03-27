#pragma once

#include "flash_log.h"
#include <vector>
#include <cstring>

/// In-memory flash simulation that enforces real NOR-flash rules:
///  - Erased state is 0xFF
///  - Write can only clear bits (1 -> 0), never set bits (0 -> 1)
///  - Erase resets an entire sector to 0xFF
///  - Erase address must be sector-aligned
///  - Reads/writes must not exceed flash bounds
class MockFlash : public IFlash {
public:
    MockFlash(size_t sectorSize, size_t sectorCount)
        : storedSectorSize(sectorSize)
        , storedSectorCount(sectorCount)
        , memory(sectorSize * sectorCount, 0xFF) // erased state
    {}

    size_t sectorSize()  const override { return storedSectorSize; }
    size_t sectorCount() const override { return storedSectorCount; }
    size_t totalSize()   const override { return storedSectorSize * storedSectorCount; }

    size_t write(size_t address, const uint8_t* data, size_t length) override {
        if (address + length > memory.size()) return 0;

        // Simulate power loss: only write partial data, then stop
        if (powerLossRemaining > 0) {
            if (length >= powerLossRemaining) {
                length = powerLossRemaining;
                powerLossRemaining = 0;
            } else {
                powerLossRemaining -= length;
            }
        } else if (powerLossTriggered) {
            return 0;
        }

        // Enforce NOR-flash rule: can only clear bits (AND with existing data)
        for (size_t i = 0; i < length; ++i) {
            memory[address + i] &= data[i];
        }
        return length;
    }

    size_t read(size_t address, uint8_t* data, size_t length) const override {
        if (address + length > memory.size()) return 0;
        std::memcpy(data, memory.data() + address, length);
        return length;
    }

    bool eraseSector(size_t sectorIndex) override {
        if (sectorIndex >= storedSectorCount) return false;

        size_t offset = sectorIndex * storedSectorSize;
        std::memset(memory.data() + offset, 0xFF, storedSectorSize);
        return true;
    }

    // --- Test helpers ---
    const uint8_t* rawMemory() const { return memory.data(); }
    uint8_t byteAt(size_t address) const { return memory[address]; }

    void corruptByte(size_t address, uint8_t value) { memory[address] = value; }

    void setPowerLossAfter(size_t bytesRemaining) {
        powerLossRemaining = bytesRemaining;
        powerLossTriggered = true;
    }

    void clearPowerLoss() {
        powerLossRemaining = 0;
        powerLossTriggered = false;
    }

private:
    size_t storedSectorSize;
    size_t storedSectorCount;
    std::vector<uint8_t> memory;
    size_t powerLossRemaining = 0;
    bool   powerLossTriggered = false;
};
