#pragma once
#include "DisplayPage.h"

class SettingsManager;
class SensorManager;

class SensorPage : public DisplayPage
{
public:
    SensorPage(SettingsManager &settings, SensorManager &sensors)
        : settingsManager(settings), sensorManager(sensors) {}

private:
    SettingsManager &settingsManager;
    SensorManager &sensorManager;

    void OnCreate() override;

    static void ClearCb(lv_event_t *e);
    static void SaveCb(lv_event_t *e);
};
