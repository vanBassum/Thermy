#pragma once
#include "DisplayPage.h"
#include "SensorManager/SensorManager.h"
#include "NetworkManager/NetworkManager.h"
#include "SettingsManager/SettingsManager.h"

class HomePage : public DisplayPage
{
public:
    HomePage(NetworkManager &net, SensorManager &sensor, SettingsManager &settings)
        : networkManager(net), sensorManager(sensor), settingsManager(settings) {}

    void Update() override;

private:
    NetworkManager &networkManager;
    SensorManager &sensorManager;
    SettingsManager &settingsManager;

    lv_obj_t *labelTime = nullptr;
    lv_obj_t *labelIP = nullptr;
    lv_obj_t *tempBoxes[4] = {};
    lv_obj_t *tempLabels[4] = {};
    lv_obj_t *chart = nullptr;
    lv_chart_series_t *chartSeries[4] = {};
    int chartCounter = 0;

    void OnCreate() override;
};
