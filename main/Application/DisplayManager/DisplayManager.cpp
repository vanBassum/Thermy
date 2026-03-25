#include "DisplayManager.h"
#include <algorithm>
#include <cinttypes>
#include <cstring>

static const lv_color_t channelColors[4] = {
    lv_palette_main(LV_PALETTE_RED),
    lv_palette_main(LV_PALETTE_BLUE),
    lv_palette_main(LV_PALETTE_GREEN),
    lv_palette_main(LV_PALETTE_YELLOW)
};
static const char *slotNames[4] = {"Red", "Blue", "Green", "Yellow"};

DisplayManager::DisplayManager(ServiceProvider &ctx)
    : sensorManager(ctx.getSensorManager())
    , homePage(ctx.getNetworkManager(), ctx.getSensorManager(), ctx.getSettingsManager())
    , wifiPage(ctx.getSettingsManager(), ctx.getNetworkManager())
    , sensorPage(ctx.getSettingsManager(), ctx.getSensorManager())
    , systemPage(ctx.getSettingsManager())
{
    auto nav = [this](const char *page) { NavigateTo(page); };
    homePage.SetNavigator(nav);
    settingsMenuPage.SetNavigator(nav);
    wifiPage.SetNavigator(nav);
    sensorPage.SetNavigator(nav);
    systemPage.SetNavigator(nav);
}

void DisplayManager::Init()
{
    auto init = initState.TryBeginInit();
    if (!init)
        return;

    ESP_LOGI(TAG, "Initializing DisplayManager...");

    lv_init();
    display.Init();

    timer.Init("LvglTickTimer", pdMS_TO_TICKS(5), true);
    timer.SetHandler([this]() { LvglTickCb(this); });
    timer.Start();

    task.Init("DisplayTask", 5, 4096);
    task.SetHandler([this]() { Work(); });
    task.Run();

    init.SetReady();
    ESP_LOGI(TAG, "DisplayManager initialized successfully.");
}

void DisplayManager::Work()
{
    NavigateTo("home");

    uint32_t delayMs;
    TickType_t lastUpdate = xTaskGetTickCount();

    while (true)
    {
        delayMs = lv_timer_handler();

        if (xTaskGetTickCount() - lastUpdate > pdMS_TO_TICKS(1000))
        {
            lastUpdate = xTaskGetTickCount();

            if (activePage)
                activePage->Update();

            // Sensor assignment popup (only on home page)
            if (activePage == &homePage && !assignPopup && sensorManager.HasPendingSensor())
                ShowAssignPopup(sensorManager.GetPendingSensorAddress());

            if (assignPopup && (xTaskGetTickCount() - popupShownAt > POPUP_TIMEOUT))
                AutoAssignToFirstEmpty();
        }

        vTaskDelay(pdMS_TO_TICKS(std::clamp(delayMs, (uint32_t)5, (uint32_t)100)));
    }
}

void DisplayManager::LvglTickCb(void *arg)
{
    (void)arg;
    lv_tick_inc(5);
}

void DisplayManager::NavigateTo(const char *page)
{
    CloseAssignPopup();

    DisplayPage *previousPage = activePage;

    if (activePage)
    {
        activePage->Hide();
        activePage = nullptr;
    }

    if (strcmp(page, "back") == 0)
    {
        // Back from sub-pages → settings menu, back from menu → home
        if (previousPage == &settingsMenuPage)
            activePage = &homePage;
        else
            activePage = &settingsMenuPage;
    }
    else if (strcmp(page, "home") == 0)
        activePage = &homePage;
    else if (strcmp(page, "settings") == 0)
        activePage = &settingsMenuPage;
    else if (strcmp(page, "wifi") == 0)
        activePage = &wifiPage;
    else if (strcmp(page, "sensors") == 0)
        activePage = &sensorPage;
    else if (strcmp(page, "system") == 0)
        activePage = &systemPage;
    else
        activePage = &homePage;

    if (activePage)
        activePage->Show(lv_scr_act());
}

// ── Assignment popup ─────────────────────────────────────────

void DisplayManager::ShowAssignPopup(uint64_t address)
{
    if (assignPopup)
        return;

    popupSensorAddress = address;
    popupShownAt = xTaskGetTickCount();

    assignPopup = lv_obj_create(lv_scr_act());
    lv_obj_set_size(assignPopup, LCD_HRES, LCD_VRES);
    lv_obj_set_pos(assignPopup, 0, 0);
    lv_obj_set_style_bg_color(assignPopup, lv_color_hex(0x101010), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(assignPopup, LV_OPA_90, LV_PART_MAIN);
    lv_obj_set_style_border_width(assignPopup, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(assignPopup, 0, LV_PART_MAIN);
    lv_obj_clear_flag(assignPopup, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(assignPopup);
    lv_label_set_text(title, "New Sensor Detected");
    lv_obj_set_style_text_color(title, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

    char addrStr[32];
    snprintf(addrStr, sizeof(addrStr), "%016" PRIX64, address);
    lv_obj_t *addrLabel = lv_label_create(assignPopup);
    lv_label_set_text(addrLabel, addrStr);
    lv_obj_set_style_text_color(addrLabel, lv_palette_main(LV_PALETTE_GREY), LV_PART_MAIN);
    lv_obj_set_style_text_font(addrLabel, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(addrLabel, LV_ALIGN_TOP_MID, 0, 50);

    lv_obj_t *subtitle = lv_label_create(assignPopup);
    lv_label_set_text(subtitle, "Assign to slot:");
    lv_obj_set_style_text_color(subtitle, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(subtitle, LV_ALIGN_TOP_MID, 0, 75);

    static constexpr lv_coord_t btnW = 100, btnH = 60, btnGap = 12;
    lv_coord_t totalW = 4 * btnW + 3 * btnGap;
    lv_coord_t startX = (LCD_HRES - totalW) / 2;

    for (int i = 0; i < 4; i++)
    {
        lv_obj_t *btn = lv_btn_create(assignPopup);
        lv_obj_set_size(btn, btnW, btnH);
        lv_obj_set_pos(btn, startX + i * (btnW + btnGap), 110);
        lv_obj_set_style_bg_color(btn, channelColors[i], LV_PART_MAIN);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_radius(btn, 10, LV_PART_MAIN);
        lv_obj_set_style_border_width(btn, 2, LV_PART_MAIN);
        lv_obj_set_style_border_color(btn, lv_color_white(), LV_PART_MAIN);
        lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);

        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text(label, slotNames[i]);
        lv_obj_set_style_text_color(label, lv_color_white(), LV_PART_MAIN);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_14, LV_PART_MAIN);
        lv_obj_center(label);

        lv_obj_set_user_data(btn, this);
        lv_obj_add_event_cb(btn, PopupEventCb, LV_EVENT_CLICKED, reinterpret_cast<void *>(i));
    }

    lv_obj_t *hint = lv_label_create(assignPopup);
    lv_label_set_text(hint, "Auto-assigns in 30s");
    lv_obj_set_style_text_color(hint, lv_palette_main(LV_PALETTE_GREY), LV_PART_MAIN);
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -20);
}

void DisplayManager::CloseAssignPopup()
{
    if (assignPopup)
    {
        lv_obj_del(assignPopup);
        assignPopup = nullptr;
        popupSensorAddress = 0;
    }
}

void DisplayManager::OnSlotSelected(int slot)
{
    if (slot < 0 || slot >= 4)
        return;
    sensorManager.AssignPendingToSlot(slot);
    CloseAssignPopup();
}

void DisplayManager::AutoAssignToFirstEmpty()
{
    for (int i = 0; i < 4; i++)
    {
        if (sensorManager.GetSlotAddress(i) == 0)
        {
            OnSlotSelected(i);
            return;
        }
    }
    sensorManager.DismissPendingSensor();
    CloseAssignPopup();
}

void DisplayManager::PopupEventCb(lv_event_t *e)
{
    int slot = reinterpret_cast<int>(lv_event_get_user_data(e));
    lv_obj_t *btn = lv_event_get_target(e);
    auto *self = static_cast<DisplayManager *>(lv_obj_get_user_data(btn));
    if (self && slot >= 0 && slot < 4)
        self->OnSlotSelected(slot);
}
