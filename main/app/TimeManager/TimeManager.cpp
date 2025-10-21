#include "TimeManager.h"
#include "WifiManager.h"
#include "esp_netif.h"           
#include "esp_netif_sntp.h"      
#include "esp_sntp.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_timer.h"


static TimeManager *s_instance = nullptr;

TimeManager::TimeManager(ServiceProvider &ctx)
    : _ctx(ctx)
{
    s_instance = this;
}

void TimeManager::Init()
{
    if (initGuard.IsReady())
        return;

    LOCK(mutex);

    ESP_LOGI(TAG, "Initializing SNTP client...");
    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
    config.sync_cb = &TimeManager::TimeSyncCallback;
    config.start = true;
    ESP_ERROR_CHECK(esp_netif_sntp_init(&config));

    initGuard.SetReady();
}

void TimeManager::TimeSyncCallback(struct timeval *tv)
{
    if (!s_instance)
        return;

    LOCK(s_instance->mutex);

    s_instance->syncUptimeUs = esp_timer_get_time();
    s_instance->syncTime = DateTime::FromUtc(tv->tv_sec);
    s_instance->synced = true;

    ESP_LOGI(TAG, "NTP synchronized: %lld → %ld (epoch)",
             static_cast<long long>(s_instance->syncUptimeUs),
             s_instance->syncTime.UtcSeconds());
}

bool TimeManager::IsTimeValid() const
{
    time_t now;
    time(&now);
    struct tm info;
    localtime_r(&now, &info);
    return (info.tm_year >= (2020 - 1900));
}

TimeSpan TimeManager::GetUptimeSinceSync() const
{
    if (!synced)
        return TimeSpan::Zero();

    uint64_t nowUs = esp_timer_get_time();
    return TimeSpan::FromSeconds((nowUs - syncUptimeUs) / 1'000'000);
}
