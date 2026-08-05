#include "SystemPage.h"
#include "SettingsManager.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

// The keys this page edits, in the order its text rows appear. SaveCb walks the
// panel's textareas in creation order and pairs them against this table, so the
// two must stay in step.
static const char *const kKeys[] = { "device.name", "ntp.server", "ntp.timezone" };
static constexpr int kKeyCount = sizeof(kKeys) / sizeof(kKeys[0]);

void SystemPage::OnCreate()
{
    AddTopBar(LV_SYMBOL_SETTINGS " System");

    char buf[64];

    ReadSettingText(settingsManager, "device.name", buf, sizeof(buf));
    AddTextRow("Name", buf, 50, 32);

    ReadSettingText(settingsManager, "ntp.server", buf, sizeof(buf));
    AddTextRow("NTP", buf, 90, 64);

    ReadSettingText(settingsManager, "ntp.timezone", buf, sizeof(buf));
    AddTextRow("Timezone", buf, 130, 64);

    lv_obj_t *saveBtn = AddButton(LV_SYMBOL_OK " Save & Reboot",
                                   lv_palette_main(LV_PALETTE_BLUE), 180, 40, SaveCb);
    lv_obj_align(saveBtn, LV_ALIGN_BOTTOM_RIGHT, -15, -15);
}

void SystemPage::SaveCb(lv_event_t *e)
{
    auto *self = static_cast<SystemPage *>(lv_event_get_user_data(e));

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
