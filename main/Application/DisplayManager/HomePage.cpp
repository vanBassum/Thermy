#include "HomePage.h"
#include "DateTime.h"
#include <cstdio>

static const lv_color_t channelColors[4] = {
    lv_palette_main(LV_PALETTE_RED),
    lv_palette_main(LV_PALETTE_BLUE),
    lv_palette_main(LV_PALETTE_GREEN),
    lv_palette_main(LV_PALETTE_YELLOW)
};

void HomePage::OnCreate()
{
    lv_obj_set_style_bg_color(panel, lv_color_black(), LV_PART_MAIN);

    // Top bar
    labelTime = lv_label_create(panel);
    lv_obj_set_pos(labelTime, 10, 6);
    lv_label_set_text(labelTime, "00:00:00");
    lv_obj_set_style_text_color(labelTime, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(labelTime, &lv_font_montserrat_14, LV_PART_MAIN);

    labelIP = lv_label_create(panel);
    lv_label_set_text(labelIP, "No IP");
    lv_obj_set_style_text_color(labelIP, lv_palette_main(LV_PALETTE_GREY), LV_PART_MAIN);
    lv_obj_set_style_text_font(labelIP, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(labelIP, LV_ALIGN_TOP_MID, 0, 6);

    // Temperature slots
    static constexpr lv_coord_t slotY = 28, slotH = 50, slotGap = 6, slotMargin = 6;
    lv_coord_t slotW = (LCD_HRES - 2 * slotMargin - 3 * slotGap) / 4;

    for (int i = 0; i < 4; i++)
    {
        lv_obj_t *box = lv_obj_create(panel);
        lv_obj_set_size(box, slotW, slotH);
        lv_obj_set_pos(box, slotMargin + i * (slotW + slotGap), slotY);
        lv_obj_set_style_bg_color(box, lv_color_hex(0x1a1a1a), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(box, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_width(box, 2, LV_PART_MAIN);
        lv_obj_set_style_border_color(box, channelColors[i], LV_PART_MAIN);
        lv_obj_set_style_radius(box, 6, LV_PART_MAIN);
        lv_obj_set_style_pad_all(box, 0, LV_PART_MAIN);
        lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *label = lv_label_create(box);
        lv_label_set_text(label, "--.--");
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_text_color(label, channelColors[i], LV_PART_MAIN);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_20, LV_PART_MAIN);

        tempBoxes[i] = box;
        tempLabels[i] = label;
    }

    // Chart
    static constexpr lv_coord_t chartY = slotY + slotH + 6;
    lv_coord_t chartH = LCD_VRES - chartY - 6;
    int32_t graphMin = settingsManager.getInt("graph.min", 0);
    int32_t graphMax = settingsManager.getInt("graph.max", 100);

    chart = lv_chart_create(panel);
    lv_obj_set_size(chart, LCD_HRES - 2 * slotMargin, chartH);
    lv_obj_set_pos(chart, slotMargin, chartY);
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    lv_chart_set_update_mode(chart, LV_CHART_UPDATE_MODE_SHIFT);
    lv_chart_set_point_count(chart, 120);
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, graphMin, graphMax);
    lv_obj_set_style_size(chart, 0, LV_PART_INDICATOR);
    lv_chart_set_div_line_count(chart, 5, 10);
    lv_obj_set_style_bg_color(chart, lv_color_hex(0x0c0c0c), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(chart, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(chart, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(chart, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_radius(chart, 6, LV_PART_MAIN);
    lv_obj_set_style_pad_left(chart, 30, LV_PART_MAIN);
    lv_obj_set_style_pad_right(chart, 4, LV_PART_MAIN);
    lv_obj_set_style_pad_top(chart, 4, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(chart, 4, LV_PART_MAIN);
    lv_obj_clear_flag(chart, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_line_color(chart, lv_color_hex(0x222222), LV_PART_MAIN);

    // Y-axis labels
    char buf[8];
    int32_t step = (graphMax - graphMin) / 5;
    for (int i = 0; i <= 5; i++)
    {
        snprintf(buf, sizeof(buf), "%ld", graphMax - i * step);
        lv_obj_t *lbl = lv_label_create(chart);
        lv_label_set_text(lbl, buf);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0x666666), LV_PART_MAIN);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_10, LV_PART_MAIN);
        lv_obj_set_pos(lbl, 2, (lv_coord_t)(i * (chartH - 8) / 5));
    }

    for (int i = 0; i < 4; i++)
        chartSeries[i] = lv_chart_add_series(chart, channelColors[i], LV_CHART_AXIS_PRIMARY_Y);

    // Gear button (on top)
    lv_obj_t *gearBtn = lv_btn_create(panel);
    lv_obj_set_size(gearBtn, 44, 26);
    lv_obj_align(gearBtn, LV_ALIGN_TOP_RIGHT, -4, 1);
    lv_obj_set_style_bg_color(gearBtn, lv_color_hex(0x282828), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(gearBtn, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(gearBtn, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(gearBtn, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(gearBtn, lv_color_hex(0x444444), LV_PART_MAIN);
    lv_obj_set_style_radius(gearBtn, 4, LV_PART_MAIN);

    lv_obj_t *gearIcon = lv_label_create(gearBtn);
    lv_label_set_text(gearIcon, LV_SYMBOL_SETTINGS);
    lv_obj_set_style_text_color(gearIcon, lv_color_white(), LV_PART_MAIN);
    lv_obj_center(gearIcon);
    lv_obj_add_event_cb(gearBtn, [](lv_event_t *e) {
        auto *page = static_cast<HomePage *>(lv_event_get_user_data(e));
        if (page->navigate)
            page->navigate("settings");
    }, LV_EVENT_CLICKED, this);
}

void HomePage::Update()
{
    char buf[32];

    DateTime now = DateTime::Now();
    now.ToStringLocal(buf, sizeof(buf), "%H:%M:%S");
    lv_label_set_text(labelTime, buf);

    auto status = networkManager.wifi().getStatus();
    if (status.has_ipv4)
        snprintf(buf, sizeof(buf), IPSTR, IP2STR(&status.ipv4.ip));
    else
        snprintf(buf, sizeof(buf), "No IP");
    lv_label_set_text(labelIP, buf);

    chartCounter++;
    if (chartCounter > 60)
        chartCounter = 0;

    for (int i = 0; i < 4; i++)
    {
        if (sensorManager.IsSlotActive(i))
        {
            float temp = sensorManager.GetTemperature(i);
            snprintf(buf, sizeof(buf), "%.1f°", temp);
            lv_label_set_text(tempLabels[i], buf);
            if (chartCounter == 0)
                lv_chart_set_next_value(chart, chartSeries[i], (lv_coord_t)temp);
        }
        else
        {
            lv_label_set_text(tempLabels[i], "--.--");
        }
    }
    lv_chart_refresh(chart);
}
