#include "NvsStorage.h"
#include "nvs_flash.h"
#include "esp_log.h"

NvsStorage::NvsStorage(const char *partition, const char *namespaceName)
    : partitionName(partition), namespaceName(namespaceName), handle(0)
{
}

esp_err_t NvsStorage::Open(NvsStorageFlags flags)
{
    if (initGuard.IsReady())
        return ESP_OK;

    esp_err_t err = InitPartition(flags);
    if (err != ESP_OK)
        return err;

    err = TryOpenNamespace(flags);
    if (err != ESP_OK)
        return err;

    ESP_LOGI(TAG, "Opened NVS (partition=%s, namespace=%s)", partitionName, namespaceName);
    initGuard.SetReady();
    return ESP_OK;
}

void NvsStorage::Close()
{
    if (handle != 0)
    {
        nvs_close(handle);
        handle = 0;
        initGuard.SetNotReady();
    }
}

esp_err_t NvsStorage::Commit()
{
    REQUIRE_READY(initGuard);
    esp_err_t err = nvs_commit(handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Commit failed err=0x%x", err);
    }
    return err;
}

// ---------------------- INT8 ----------------------
esp_err_t NvsStorage::GetInt8(const char *key, int8_t &value)
{
    REQUIRE_READY(initGuard);
    return nvs_get_i8(handle, key, &value);
}

esp_err_t NvsStorage::SetInt8(const char *key, int8_t value)
{
    REQUIRE_READY(initGuard);
    return nvs_set_i8(handle, key, value);
}

// ---------------------- UINT8 ----------------------
esp_err_t NvsStorage::GetUInt8(const char *key, uint8_t &value)
{
    REQUIRE_READY(initGuard);
    return nvs_get_u8(handle, key, &value);
}

esp_err_t NvsStorage::SetUInt8(const char *key, uint8_t value)
{
    REQUIRE_READY(initGuard);
    return nvs_set_u8(handle, key, value);
}

// ---------------------- INT16 ----------------------
esp_err_t NvsStorage::GetInt16(const char *key, int16_t &value)
{
    REQUIRE_READY(initGuard);
    return nvs_get_i16(handle, key, &value);
}

esp_err_t NvsStorage::SetInt16(const char *key, int16_t value)
{
    REQUIRE_READY(initGuard);
    return nvs_set_i16(handle, key, value);
}

// ---------------------- UINT16 ----------------------
esp_err_t NvsStorage::GetUInt16(const char *key, uint16_t &value)
{
    REQUIRE_READY(initGuard);
    return nvs_get_u16(handle, key, &value);
}

esp_err_t NvsStorage::SetUInt16(const char *key, uint16_t value)
{
    REQUIRE_READY(initGuard);
    return nvs_set_u16(handle, key, value);
}

// ---------------------- INT32 ----------------------
esp_err_t NvsStorage::GetInt32(const char *key, int32_t &value)
{
    REQUIRE_READY(initGuard);
    return nvs_get_i32(handle, key, &value);
}

esp_err_t NvsStorage::SetInt32(const char *key, int32_t value)
{
    REQUIRE_READY(initGuard);
    return nvs_set_i32(handle, key, value);
}

// ---------------------- UINT32 ----------------------
esp_err_t NvsStorage::GetUInt32(const char *key, uint32_t &value)
{
    REQUIRE_READY(initGuard);
    return nvs_get_u32(handle, key, &value);
}

esp_err_t NvsStorage::SetUInt32(const char *key, uint32_t value)
{
    REQUIRE_READY(initGuard);
    return nvs_set_u32(handle, key, value);
}

// ---------------------- INT64 ----------------------
esp_err_t NvsStorage::GetInt64(const char *key, int64_t &value)
{
    REQUIRE_READY(initGuard);
    return nvs_get_i64(handle, key, &value);
}

esp_err_t NvsStorage::SetInt64(const char *key, int64_t value)
{
    REQUIRE_READY(initGuard);
    return nvs_set_i64(handle, key, value);
}

// ---------------------- UINT64 ----------------------
esp_err_t NvsStorage::GetUInt64(const char *key, uint64_t &value)
{
    REQUIRE_READY(initGuard);
    return nvs_get_u64(handle, key, &value);
}

esp_err_t NvsStorage::SetUInt64(const char *key, uint64_t value)
{
    REQUIRE_READY(initGuard);
    return nvs_set_u64(handle, key, value);
}


// ---------------------- Boolean ----------------------

esp_err_t NvsStorage::GetBool(const char *key, bool &value)
{
    REQUIRE_READY(initGuard);
    uint8_t v = 0;
    esp_err_t err = nvs_get_u8(handle, key, &v);
    if (err == ESP_OK) value = (v != 0);
    return err;
}

esp_err_t NvsStorage::SetBool(const char *key, bool value)
{
    REQUIRE_READY(initGuard);
    return nvs_set_u8(handle, key, value ? 1 : 0);
}

// ---------------------- Float ----------------------

esp_err_t NvsStorage::GetFloat(const char *key, float &value)
{
    REQUIRE_READY(initGuard);
    size_t size = sizeof(value);
    esp_err_t err = nvs_get_blob(handle, key, &value, &size);
    if (err == ESP_OK && size != sizeof(value))
    {
        ESP_LOGE(TAG, "GetFloat: size mismatch (got %u, expected %u)",
                 static_cast<unsigned>(size),
                 static_cast<unsigned>(sizeof(value)));
        return ESP_ERR_INVALID_SIZE;
    }
    return err;
}

esp_err_t NvsStorage::SetFloat(const char *key, float value)
{
    REQUIRE_READY(initGuard);
    return nvs_set_blob(handle, key, &value, sizeof(value));
}

// ---------------------- String ----------------------

esp_err_t NvsStorage::GetString(const char *key, char *buffer, size_t bufferSize, size_t &dataSize)
{
    REQUIRE_READY(initGuard);
    dataSize = bufferSize;
    return nvs_get_str(handle, key, buffer, &dataSize);
}

esp_err_t NvsStorage::SetString(const char *key, const char *value)
{
    REQUIRE_READY(initGuard);
    return nvs_set_str(handle, key, value);
}

// ---------------------- Blob ----------------------

esp_err_t NvsStorage::GetBlob(const char *key, void *buffer, size_t bufferSize, size_t &dataSize)
{
    REQUIRE_READY(initGuard);
    dataSize = bufferSize;
    return nvs_get_blob(handle, key, buffer, &dataSize);
}

esp_err_t NvsStorage::SetBlob(const char *key, const void *buffer, size_t dataSize)
{
    REQUIRE_READY(initGuard);
    return nvs_set_blob(handle, key, buffer, dataSize);
}

// ---------------------- Utilities ----------------------

void NvsStorage::InitNvsPartition(const char *partition)
{
    esp_err_t err = nvs_flash_init_partition(partition);
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase_partition(partition));
        err = nvs_flash_init_partition(partition);
    }
    ESP_ERROR_CHECK(err);
}

void NvsStorage::PrintStats(const char *partition)
{
    nvs_stats_t stats;
    esp_err_t err = nvs_get_stats(partition, &stats);
    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "NVS partition '%s' stats:", partition);
        ESP_LOGI(TAG, "  Used entries: %d", stats.used_entries);
        ESP_LOGI(TAG, "  Free entries: %d", stats.free_entries);
        ESP_LOGI(TAG, "  Total entries: %d", stats.total_entries);
        ESP_LOGI(TAG, "  Namespace count: %d", stats.namespace_count);
    }
    else
    {
        ESP_LOGE(TAG, "Failed to get NVS stats for partition '%s' err=0x%x", partition, err);
    }
}

esp_err_t NvsStorage::InitPartition(NvsStorageFlags flags)
{
    esp_err_t err = nvs_flash_init_partition(partitionName);

    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        if (HasFlag(flags, NvsStorageFlags::FormatIfCorrupted))
        {
            ESP_LOGW(TAG, "Formatting NVS partition '%s' (reason: %s)",
                     partitionName, esp_err_to_name(err));
            err = HandlePartitionError();
        }
    }

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize NVS partition '%s': %s (0x%x)",
                 partitionName, esp_err_to_name(err), err);
    }

    return err;
}

esp_err_t NvsStorage::TryOpenNamespace(NvsStorageFlags flags)
{
    esp_err_t err = nvs_open_from_partition(partitionName, namespaceName, NVS_READWRITE, &handle);
    if (err == ESP_ERR_NVS_NOT_FOUND && (HasFlag(flags, NvsStorageFlags::CreateIfMissing)))
    {
        ESP_LOGW(TAG, "Namespace '%s' not found in partition '%s'. Retrying (auto-create)...",
                 namespaceName, partitionName);
        err = nvs_open_from_partition(partitionName, namespaceName, NVS_READWRITE, &handle);
    }

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to open NVS namespace '%s' (partition=%s): %s (0x%x)",
                 namespaceName, partitionName, esp_err_to_name(err), err);
    }

    return err;
}

esp_err_t NvsStorage::HandlePartitionError()
{
    esp_err_t err = nvs_flash_erase_partition(partitionName);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to erase partition '%s': %s (0x%x)",
                 partitionName, esp_err_to_name(err), err);
        return err;
    }

    err = nvs_flash_init_partition(partitionName);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to reinitialize partition '%s' after erase: %s (0x%x)",
                 partitionName, esp_err_to_name(err), err);
    }

    return err;
}