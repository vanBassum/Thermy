#include "SettingsMenuPage.h"
#include "esp_system.h"

void SettingsMenuPage::OnCreate()
{
    AddTopBar(LV_SYMBOL_SETTINGS " Settings");

    static constexpr lv_coord_t btnW = 105, btnH = 80, gap = 10;
    lv_coord_t totalW = 4 * btnW + 3 * gap;
    lv_coord_t startX = (LCD_HRES - totalW) / 2;
    lv_coord_t y = 65;

    auto navTo = [](lv_event_t *e) {
        auto *page = static_cast<SettingsMenuPage *>(lv_event_get_user_data(e));
        lv_obj_t *btn = lv_event_get_target(e);
        auto *target = static_cast<const char *>(lv_obj_get_user_data(btn));
        if (page->navigate && target)
            page->navigate(target);
    };

    auto makeTile = [&](const char *text, lv_color_t color, int idx, const char *target) {
        lv_obj_t *btn = AddButton(text, color, btnW, btnH, navTo);
        lv_obj_set_pos(btn, startX + idx * (btnW + gap), y);
        lv_obj_set_user_data(btn, const_cast<char *>(target));
    };

    makeTile(LV_SYMBOL_WIFI "\nWiFi", lv_palette_main(LV_PALETTE_BLUE), 0, "wifi");
    makeTile(LV_SYMBOL_EYE_OPEN "\nSensors", lv_palette_main(LV_PALETTE_GREEN), 1, "sensors");
    makeTile(LV_SYMBOL_IMAGE "\nGraph", lv_palette_main(LV_PALETTE_ORANGE), 2, "graph");
    makeTile(LV_SYMBOL_SETTINGS "\nSystem", lv_palette_main(LV_PALETTE_GREY), 3, "system");

    lv_obj_t *rebootBtn = AddButton(LV_SYMBOL_POWER " Reboot",
                                     lv_palette_main(LV_PALETTE_RED), 140, 40,
                                     [](lv_event_t *) {
                                         vTaskDelay(pdMS_TO_TICKS(200));
                                         esp_restart();
                                     }, nullptr);
    lv_obj_align(rebootBtn, LV_ALIGN_BOTTOM_MID, 0, -15);
}
