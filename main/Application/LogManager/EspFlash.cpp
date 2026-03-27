#include "EspFlash.h"
#include "esp_log.h"
#include <cstring>

static constexpr const char* TAG = "EspFlash";
static constexpr size_t ESP_FLASH_SECTOR_SIZE = 4096;

bool EspFlash::mount(const char* partitionLabel)
{
    partition_ = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, partitionLabel);

    if (!partition_)
    {
        ESP_LOGE(TAG, "Partition '%s' not found", partitionLabel);
        return false;
    }

    sectorSize_ = ESP_FLASH_SECTOR_SIZE;
    totalSize_ = partition_->size;
    sectorCount_ = totalSize_ / sectorSize_;

    ESP_LOGI(TAG, "Mounted '%s': %u sectors of %u bytes (%u KB total)",
             partitionLabel,
             (unsigned)sectorCount_, (unsigned)sectorSize_,
             (unsigned)(totalSize_ / 1024));

    return true;
}

size_t EspFlash::write(size_t address, const uint8_t* data, size_t length)
{
    if (address + length > totalSize_) return 0;

    esp_err_t err = esp_partition_write_raw(partition_, address, data, length);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Write failed at 0x%x len %u: %s",
                 (unsigned)address, (unsigned)length, esp_err_to_name(err));
        return 0;
    }
    return length;
}

size_t EspFlash::read(size_t address, uint8_t* data, size_t length) const
{
    if (address + length > totalSize_) return 0;

    esp_err_t err = esp_partition_read_raw(partition_, address, data, length);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Read failed at 0x%x len %u: %s",
                 (unsigned)address, (unsigned)length, esp_err_to_name(err));
        return 0;
    }
    return length;
}

bool EspFlash::eraseSector(size_t sectorIndex)
{
    if (sectorIndex >= sectorCount_) return false;

    size_t offset = sectorIndex * sectorSize_;
    esp_err_t err = esp_partition_erase_range(partition_, offset, sectorSize_);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Erase sector %u failed: %s",
                 (unsigned)sectorIndex, esp_err_to_name(err));
        return false;
    }
    return true;
}
