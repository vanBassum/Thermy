#pragma once
#include "DisplayPage.h"

class SettingsManager;

class GraphPage : public DisplayPage
{
public:
    GraphPage(SettingsManager &settings) : settingsManager(settings) {}

private:
    SettingsManager &settingsManager;

    void OnCreate() override;

    static void SaveCb(lv_event_t *e);
};
