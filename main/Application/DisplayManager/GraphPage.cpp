#include "GraphPage.h"
#include "SettingsManager/SettingsManager.h"
#include "TemperatureHistory/TemperatureHistory.h"
#include <cstdio>
#include <cstdlib>

void GraphPage::OnCreate()
{
    AddTopBar(LV_SYMBOL_IMAGE " Graph");

    char buf[16];

    snprintf(buf, sizeof(buf), "%ld", settingsManager.getInt("history.rate", 10));
    lv_obj_t *rateTa = AddTextRow("Sample (s)", buf, 50, 6);

    // Duration hint
    int32_t rate = settingsManager.getInt("history.rate", 10);
    int32_t totalSec = TemperatureHistory::MAX_SAMPLES * rate;
    char durationBuf[48];
    if (totalSec < 3600)
        snprintf(durationBuf, sizeof(durationBuf), "Storage: %ldm (%d samples)", totalSec / 60, (int)TemperatureHistory::MAX_SAMPLES);
    else if (totalSec < 86400)
        snprintf(durationBuf, sizeof(durationBuf), "Storage: %.1fh (%d samples)", totalSec / 3600.0f, (int)TemperatureHistory::MAX_SAMPLES);
    else
        snprintf(durationBuf, sizeof(durationBuf), "Storage: %.1fd (%d samples)", totalSec / 86400.0f, (int)TemperatureHistory::MAX_SAMPLES);

    lv_obj_t *durationLabel = lv_label_create(panel);
    lv_label_set_text(durationLabel, durationBuf);
    lv_obj_set_style_text_color(durationLabel, lv_palette_main(LV_PALETTE_GREEN), LV_PART_MAIN);
    lv_obj_set_style_text_font(durationLabel, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_pos(durationLabel, 130, 86);

    snprintf(buf, sizeof(buf), "%ld", settingsManager.getInt("graph.min", 0));
    AddTextRow("Y Min", buf, 110, 6);

    snprintf(buf, sizeof(buf), "%ld", settingsManager.getInt("graph.max", 100));
    AddTextRow("Y Max", buf, 150, 6);

    lv_obj_t *saveBtn = AddButton(LV_SYMBOL_OK " Save & Reboot",
                                   lv_palette_main(LV_PALETTE_BLUE), 180, 40, SaveCb);
    lv_obj_align(saveBtn, LV_ALIGN_BOTTOM_RIGHT, -15, -15);
}

void GraphPage::SaveCb(lv_event_t *e)
{
    auto *self = static_cast<GraphPage *>(lv_event_get_user_data(e));

    static const char *keys[] = {"history.rate", "graph.min", "graph.max"};
    int keyIdx = 0;
    uint32_t count = lv_obj_get_child_cnt(self->panel);
    for (uint32_t i = 0; i < count && keyIdx < 3; i++)
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
