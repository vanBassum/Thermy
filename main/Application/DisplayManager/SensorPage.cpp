#include "SensorPage.h"
#include "SettingsManager/SettingsManager.h"
#include "SensorManager/SensorManager.h"
#include <cstdio>
#include <cstdlib>

void SensorPage::OnCreate()
{
    AddTopBar(LV_SYMBOL_EYE_OPEN " Sensors");

    char buf[16];

    snprintf(buf, sizeof(buf), "%ld", settingsManager.getInt("sensor.scan", 5000));
    AddTextRow("Scan (ms)", buf, 50, 8);

    snprintf(buf, sizeof(buf), "%ld", settingsManager.getInt("sensor.read", 1000));
    AddTextRow("Read (ms)", buf, 90, 8);

    lv_obj_t *clearBtn = AddButton(LV_SYMBOL_TRASH " Clear All Assignments",
                                    lv_palette_main(LV_PALETTE_DEEP_ORANGE), 220, 40, ClearCb);
    lv_obj_align(clearBtn, LV_ALIGN_BOTTOM_LEFT, 15, -15);

    lv_obj_t *saveBtn = AddButton(LV_SYMBOL_OK " Save & Reboot",
                                   lv_palette_main(LV_PALETTE_BLUE), 180, 40, SaveCb);
    lv_obj_align(saveBtn, LV_ALIGN_BOTTOM_RIGHT, -15, -15);
}

void SensorPage::ClearCb(lv_event_t *e)
{
    auto *self = static_cast<SensorPage *>(lv_event_get_user_data(e));
    self->sensorManager.ClearAllSlots();
    if (self->navigate)
        self->navigate("back");
}

void SensorPage::SaveCb(lv_event_t *e)
{
    auto *self = static_cast<SensorPage *>(lv_event_get_user_data(e));

    static const char *keys[] = {"sensor.scan", "sensor.read"};
    int keyIdx = 0;
    uint32_t count = lv_obj_get_child_cnt(self->panel);
    for (uint32_t i = 0; i < count && keyIdx < 2; i++)
    {
        lv_obj_t *child = lv_obj_get_child(self->panel, i);
        if (lv_obj_check_type(child, &lv_textarea_class))
        {
            self->settingsManager.setInt(keys[keyIdx], atoi(lv_textarea_get_text(child)));
            keyIdx++;
        }
    }

    self->SaveAndReboot(self->settingsManager);
}
