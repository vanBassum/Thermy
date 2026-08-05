#include "GraphPage.h"
#include "SettingsManager.h"
#include <cinttypes>
#include <cstdio>
#include <cstdlib>

// Edited keys in text-row order — SaveCb pairs textareas against this table.
//
// This page used to configure the on-flash history log (sample rate, retention).
// That log is gone: time-series data now leaves the device as telemetry, and the
// graphing happens wherever those points land. What is left here is what the
// *device's own* chart needs — its Y range — plus a read-only note of how often
// points are published, which SensorManager owns.
static const char *const kKeys[] = { "graph.min", "graph.max" };
static constexpr int kKeyCount = sizeof(kKeys) / sizeof(kKeys[0]);

void GraphPage::OnCreate()
{
    AddTopBar(LV_SYMBOL_IMAGE " Graph");

    char buf[16];

    ReadSettingText(settingsManager, "graph.min", buf, sizeof(buf));
    AddTextRow("Y Min", buf, 50, 6);

    ReadSettingText(settingsManager, "graph.max", buf, sizeof(buf));
    AddTextRow("Y Max", buf, 90, 6);

    // Where the long-term history actually goes now.
    int32_t telemetrySec = ReadSettingInt(settingsManager, "sensor.telem", 60);
    char note[64];
    snprintf(note, sizeof(note), "History: telemetry every %" PRId32 "s", telemetrySec);

    lv_obj_t *noteLabel = lv_label_create(panel);
    lv_label_set_text(noteLabel, note);
    lv_obj_set_style_text_color(noteLabel, lv_palette_main(LV_PALETTE_GREEN), LV_PART_MAIN);
    lv_obj_set_style_text_font(noteLabel, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_pos(noteLabel, 12, 140);

    lv_obj_t *saveBtn = AddButton(LV_SYMBOL_OK " Save & Reboot",
                                   lv_palette_main(LV_PALETTE_BLUE), 180, 40, SaveCb);
    lv_obj_align(saveBtn, LV_ALIGN_BOTTOM_RIGHT, -15, -15);
}

void GraphPage::SaveCb(lv_event_t *e)
{
    auto *self = static_cast<GraphPage *>(lv_event_get_user_data(e));

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
