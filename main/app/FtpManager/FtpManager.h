#pragma once
#include "esp_log.h"
#include "FtpServer.h"
#include "rtos.h"
#include "ServiceProvider.h"



class FtpManager {
    constexpr static const char* TAG = "FtpManager";
    constexpr static const char* rootPath = "/fat";

public:
    FtpManager(ServiceProvider& provider){}
    ~FtpManager() = default;
    FtpManager(const FtpManager &) = delete;
    FtpManager &operator=(const FtpManager &) = delete;

    void Init()
    {
        if(initGuard.IsReady())
            return;

        ftpServer.init();
            
        task.Init("FtpManager", 7, 4096);
        task.SetHandler([this]() {Work();});
        task.Run();

        initGuard.SetReady();
    }

private:
    InitGuard initGuard;
    FtpServer ftpServer {rootPath};
    Task task;

    void Work() {
        while(1)
        {
            ftpServer.tick();
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
};
