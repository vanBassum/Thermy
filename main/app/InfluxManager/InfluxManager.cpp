#include "InfluxManager.h"
#include "core_utils.h"

InfluxManager::InfluxManager(ServiceProvider &services)
    : settingsManager(services.GetSettingsManager()), dataManager(services.GetDataManager()), wifiManager(services.GetWifiManager()), timeManager(services.GetTimeManager())
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
        memcpy(influxBucket, settings.system.influxBucket, MIN(sizeof(influxBucket), sizeof(settings.system.influxBucket))); });

    _lastWriteTime = NowMs();

    task.Init("InfluxManager", 7, 1024 * 4);
    task.SetHandler([&]()
                    { Work(); });
    task.Run();

    _initGuard.SetReady();
}

void InfluxManager::Tick(TickContext &ctx)
{
    REQUIRE_READY(_initGuard);
    LOCK(_mutex);

    if (_state.Get() == State::Working)
    {
        ctx.RequestNoSleep();
        return; // Still working
    }

    if (!ctx.HasElapsed(_lastWriteTime, INFLUX_WRITE_INTERVAL))
        return;

    if (!wifiManager.IsConnected())
    {
        ctx.RequestNoSleep(); // Prevent sleep for wifi to reconnect
        return;
    }

    if (!timeManager.IsTimeValid())
    {
        ctx.RequestNoSleep(); // Prevent sleep for ntp to sync
        return;
    }

    task.Notify(1);
    ctx.MarkExecuted(_lastWriteTime, INFLUX_WRITE_INTERVAL);
    ctx.RequestNoSleep();
}

void InfluxManager::Work()
{
        return;
    InfluxClient _client;
    _client.Init(influxBaseUrl, influxApiKey, influxOrganisation, influxBucket);

    while (1)
    {
        uint32_t events;
        task.NotifyWait(&events);
        _state.Set(State::Working);
        InfluxSession session = _client.CreateSession(pdMS_TO_TICKS(5000));
        int entriesWritten = 0;

        DataManager::Iterator it = dataManager.GetIterator();

        DataEntry entry;
        while (it.Read(entry))
        {
            if (HasFlag(entry.flags, HandledFlags::WrittenToInflux))
            {
                it.Advance();
                continue;
            }

            EnsureValidTimestamp(entry);

            auto logCodePair = entry.FindPair(DataKey::LogCode);
            LogCode logCode = logCodePair ? logCodePair->value.asLogCode : LogCode::None;

            switch (logCode)
            {
            case LogCode::TemperatureRead:
                SendLogCode_Temperature(session, entry);
                break;
            default:
                ESP_LOGW(TAG, "Unknown log code: %d", static_cast<int>(logCode));
                it.Advance();
                continue;
            }

            entry.flags = SetFlag(entry.flags, HandledFlags::WrittenToInflux);
            if (!it.Write(entry))
                ESP_LOGW(TAG, "Failed to write flag for index %d", it.GetIndex());

            it.Advance();
        }

        // only after the loop:
        session.Finish();
        _state.Set(State::Idle);
    }
}

void InfluxManager::EnsureValidTimestamp(DataEntry &entry)
{
    // Timestamp when i wrote this code.
    if (entry.timestamp.UtcSeconds() > 1761148729)
        return; // Valid timestamp

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
