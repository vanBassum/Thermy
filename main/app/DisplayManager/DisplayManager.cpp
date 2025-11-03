#include "DisplayManager.h"

DisplayManager::DisplayManager(ServiceProvider &ctx)
{
}

void DisplayManager::Init()
{
    if (initGuard.IsReady())
        return;
    LOCK(mutex);

    display.Init();

    // Example: add your LVGL UI logic here
    lv_obj_t *label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "Hello from DisplayManager 👋");
    lv_obj_center(label);

    initGuard.SetReady();
}
