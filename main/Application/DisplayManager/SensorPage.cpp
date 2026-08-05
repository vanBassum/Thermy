#include "SensorPage.h"
#include "SettingsManager.h"
#include "SensorManager.h"
#include <cstdio>
#include <cstdlib>

// Edited keys in text-row order — SaveCb pairs textareas against this table.
static const char *const kKeys[] = { "sensor.scan", "sensor.read", "sensor.telem" };
static constexpr int kKeyCount = sizeof(kKeys) / sizeof(kKeys[0]);

void SensorPage::OnCreate()
{
    AddTopBar(LV_SYMBOL_EYE_OPEN " Sensors");

    char buf[16];

    ReadSettingText(settingsManager, "sensor.scan", buf, sizeof(buf));
    AddTextRow("Scan (ms)", buf, 50, 8);

    ReadSettingText(settingsManager, "sensor.read", buf, sizeof(buf));
    AddTextRow("Read (ms)", buf, 90, 8);

    ReadSettingText(settingsManager, "sensor.telem", buf, sizeof(buf));
    AddTextRow("Telemetry (s)", buf, 130, 8);

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

    int keyIdx = 0;
    uint32_t count = lv_obj_get_child_cnt(self->panel);
    for (uint32_t i = 0; i < count && keyIdx < kKeyCount; i++)
    {
        lv_obj_t *child = lv_obj_get_child(self->panel, i);
        if (lv_obj_check_type(child, &lv_textarea_class))
        {
            WriteSettingText(self->settingsManager, kKeys[keyIdx],
                             lv_textarea_get_text(child));
            keyIdx++;
        }
    }

    self->SaveAndReboot(self->settingsManager);
}
