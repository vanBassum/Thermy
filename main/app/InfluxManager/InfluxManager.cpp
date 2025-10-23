#include "InfluxManager.h"
#include "core_utils.h"

InfluxManager::InfluxManager(ServiceProvider &services)
    : settingsManager(services.GetSettingsManager())
    , dataManager(services.GetDataManager())
    , wifiManager(services.GetWifiManager())
    , timeManager(services.GetTimeManager())
{
}

void InfluxManager::Init()
{
    if (_initGuard.IsReady())
        return;

    LOCK(_mutex);

    settingsManager.Access([this](const RootSettings &settings)
    {
        memcpy(influxBaseUrl, settings.system.influxBaseUrl, MIN(sizeof(influxBaseUrl), sizeof(settings.system.influxBaseUrl)));
        memcpy(influxApiKey, settings.system.influxApiKey, MIN(sizeof(influxApiKey), sizeof(settings.system.influxApiKey)));
        memcpy(influxOrganisation, settings.system.influxOrganisation, MIN(sizeof(influxOrganisation), sizeof(settings.system.influxOrganisation)));
        memcpy(influxBucket, settings.system.influxBucket, MIN(sizeof(influxBucket), sizeof(settings.system.influxBucket)));
    });

    
    _lastWriteTime = NowMs();

    task.Init("InfluxManager", 7, 1024 * 4);
    task.SetHandler([&]() { Work(); });
    task.Run();

    _initGuard.SetReady();
}

void InfluxManager::Tick(TickContext &ctx)
{
    REQUIRE_READY(_initGuard);
    LOCK(_mutex);

    if (_state.Get() == State::Working)
    {
        ctx.PreventSleep();
        return; // Still working
    }

    if (!ctx.HasElapsed(_lastWriteTime, INFLUX_WRITE_INTERVAL))
        return;

    if (!wifiManager.IsConnected()) {
        ctx.PreventSleep(); // Prevent sleep for wifi to reconnect
        return;
    }

    if (!timeManager.IsTimeValid()){
        ctx.PreventSleep(); // Prevent sleep for ntp to sync
        return;
    }

    // Start task
    task.Notify(1);
    _lastWriteTime = ctx.TimeSinceBoot();
    ctx.PreventSleep();
}

void InfluxManager::EnsureValidTimestamp(DataEntry &entry)
{
    // Timestamp when i wrote this code.
    if(entry.timestamp.UtcSeconds() > 1761148729)
        return; // Valid timestamp

    if(!timeManager.HasSynced())
        return; // Cannot fix timestamp without valid time
    
    // Fix the timestamp
    entry.timestamp += timeManager.GetUptimeSinceFirstSync();
}

void InfluxManager::SendLogCode_Temperature(InfluxSession &session, DataEntry &entry)
{
    auto valuePair = entry.FindPair(DataKey::Value);
    auto addressPair = entry.FindPair(DataKey::Address);

    if (!valuePair || !addressPair)
    {
        ESP_LOGW(TAG, "Temperature entry missing value or address.");
        return;
    }
    
    char addressStr[17] = {};
    snprintf(addressStr, sizeof(addressStr), "%016llX", addressPair->value.asUint64);

    char timestampStr[25] = {};
    entry.timestamp.ToStringLocal(timestampStr, sizeof(timestampStr), DateTime::FormatIso8601);

    session.withMeasurement("temperature", entry.timestamp)
            .withTag("address", addressStr)
            .withField("value", valuePair->value.asFloat);
}

void InfluxManager::Work()
{
    InfluxClient _client;
    _client.Init(influxBaseUrl, influxApiKey, influxOrganisation, influxBucket);

    while(1)
    {
        uint32_t events;
        task.NotifyWait(&events);
        _state.Set(State::Working);

        InfluxSession session = _client.CreateSession(pdMS_TO_TICKS(5000));
        int entriesWritten = 0;

        dataManager.ForEach([&](DataEntry &entry)
        {
            if(HasFlag(entry.flags, HandledFlags::WrittenToInflux))
                return; // Already written

            EnsureValidTimestamp(entry);

            // Find the logcode
            auto logCodePair = entry.FindPair(DataKey::LogCode);
            LogCode logCode = logCodePair ? logCodePair->value.asLogCode : LogCode::None;

            switch (logCode)
            {
            case LogCode::TemperatureRead:
                SendLogCode_Temperature(session, entry);
                break;
            
            default:
                ESP_LOGW(TAG, "Unknown log code: %d", static_cast<int>(logCode));
                return;
            }

            // Mark as written
            entry.flags = SetFlag(entry.flags, HandledFlags::WrittenToInflux);
            entriesWritten++;
        });

        session.Finish();
        //ESP_LOGI(TAG, "InfluxDB write complete, %d entries written.", entriesWritten);


        _state.Set(State::Idle);
    }
}
