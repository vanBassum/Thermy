#pragma once
#include "nvs_flash.h"
#include "nvs.h"
#include "InitGuard.h"
#include "EnumFlags.h"

enum class NvsStorageFlags : uint32_t
{
    None             = 0,
    CreateIfMissing  = 1 << 0,  // Create partition/namespace if missing
    FormatIfCorrupted= 1 << 1,  // Erase and reinit if NVS corrupted or full
};

ENABLE_BITMASK_OPERATORS(NvsStorageFlags);

class NvsStorage
{
    constexpr static const char *TAG = "NvsStorage";

public:
    NvsStorage(const char *partition, const char *namespaceName);

    esp_err_t Open(NvsStorageFlags flags = NvsStorageFlags::None);
    void Close();
    esp_err_t Commit();

    // ---------------------- Integer ----------------------
    esp_err_t GetInt8(const char *key, int8_t &value);
    esp_err_t SetInt8(const char *key, int8_t value);

    esp_err_t GetUInt8(const char *key, uint8_t &value);
    esp_err_t SetUInt8(const char *key, uint8_t value);

    esp_err_t GetInt16(const char *key, int16_t &value);
    esp_err_t SetInt16(const char *key, int16_t value);

    esp_err_t GetUInt16(const char *key, uint16_t &value);
    esp_err_t SetUInt16(const char *key, uint16_t value);

    esp_err_t GetInt32(const char *key, int32_t &value);
    esp_err_t SetInt32(const char *key, int32_t value);

    esp_err_t GetUInt32(const char *key, uint32_t &value);
    esp_err_t SetUInt32(const char *key, uint32_t value);

    esp_err_t GetInt64(const char *key, int64_t &value);
    esp_err_t SetInt64(const char *key, int64_t value);

    esp_err_t GetUInt64(const char *key, uint64_t &value);
    esp_err_t SetUInt64(const char *key, uint64_t value);

    // ---------------------- Boolean ----------------------
    esp_err_t GetBool(const char *key, bool &value);
    esp_err_t SetBool(const char *key, bool value);

    // ---------------------- Float ----------------------
    esp_err_t GetFloat(const char *key, float &value);
    esp_err_t SetFloat(const char *key, float value);

    // ---------------------- String ----------------------
    esp_err_t GetString(const char *key, char *buffer, size_t bufferSize, size_t &dataSize);
    esp_err_t SetString(const char *key, const char *value);

    // ---------------------- Blob ----------------------
    esp_err_t GetBlob(const char *key, void *buffer, size_t bufferSize, size_t &dataSize);
    esp_err_t SetBlob(const char *key, const void *buffer, size_t dataSize);

    // ---------------------- Utilities ----------------------
    static void InitNvsPartition(const char *partition);
    static void PrintStats(const char *partition);

private:
    InitGuard initGuard;
    const char *partitionName;
    const char *namespaceName;
    nvs_handle_t handle;


    esp_err_t InitPartition(NvsStorageFlags flags);
    esp_err_t TryOpenNamespace(NvsStorageFlags flags);
    esp_err_t HandlePartitionError();
};
