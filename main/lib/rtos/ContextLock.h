#pragma once
#include "IMutex.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <cassert>

#define LOCK(mutex) ContextLock lock(mutex, __FILE__, __LINE__)

class ContextLock
{
    constexpr static const char* TAG = "ContextLock";
    constexpr static TickType_t TIMEOUT_TICKS = pdMS_TO_TICKS(1000);
    const IMutex& mutex;
    bool taken = false;
    const char* file;
    int line;

public:
    ContextLock(const IMutex& mutex, const char* file, int line)
        : mutex(mutex), file(file), line(line)
    {
        do
        {
            taken = mutex.Take(pdMS_TO_TICKS(TIMEOUT_TICKS));
            if (!taken)
            {
                const char* taskName = pcTaskGetName(nullptr);
                ESP_LOGW(TAG, "Timeout waiting for mutex at %s:%d (task: %s)", file, line, taskName ? taskName : "unknown");
            }
        } while (!taken);
    }

    ~ContextLock()
    {
        if (taken)
            mutex.Give();
    }
};
