#pragma once

#include "flash_log.h"
#include "esp_partition.h"

/// IFlash implementation backed by an ESP-IDF partition.
class EspFlash : public IFlash {
public:
    bool mount(const char* partitionLabel);

    size_t sectorSize()  const override { return sectorSize_; }
    size_t sectorCount() const override { return sectorCount_; }
    size_t totalSize()   const override { return totalSize_; }

    size_t write(size_t address, const uint8_t* data, size_t length) override;
    size_t read(size_t address, uint8_t* data, size_t length) const override;
    bool eraseSector(size_t sectorIndex) override;

private:
    const esp_partition_t* partition_ = nullptr;
    size_t sectorSize_ = 0;
    size_t sectorCount_ = 0;
    size_t totalSize_ = 0;
};
