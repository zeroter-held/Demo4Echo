#include "ui_FocusPage.h"

#include <stdio.h>

#define FOCUS_DURATION_SECONDS (25 * 60)

static uint32_t focus_remaining_seconds;
static bool focus_running;
static lv_timer_t *focus_timer;
static lv_obj_t *focus_time_label;
static lv_obj_t *focus_status_label;
static lv_obj_t *focus_action_label;

static void focus_update_view(void)
{
    char time_text[6];
    uint32_t minutes = focus_remaining_seconds / 60;
    uint32_t seconds = focus_remaining_seconds % 60;

    snprintf(time_text, sizeof(time_text), "%02lu:%02lu", (unsigned long)minutes, (unsigned long)seconds);
    lv_label_set_text(focus_time_label, time_text);

    if(focus_running) {
        lv_label_set_text(focus_status_label, "专注进行中");
        lv_label_set_text(focus_action_label, "暂停");
    } else if(focus_remaining_seconds == 0) {
        lv_label_set_text(focus_status_label, "本轮专注完成");
        lv_label_set_text(focus_action_label, "再开始");
    } else {
        lv_label_set_text(focus_status_label, "准备开始");
        lv_label_set_text(focus_action_label, "开始");
    }
}

static void focus_timer_cb(lv_timer_t *timer)
{
    (void)timer;

    if(!focus_running) {
        return;
    }

    if(focus_remaining_seconds > 0) {
        focus_remaining_seconds--;
    }

    if(focus_remaining_seconds == 0) {
        focus_running = false;
        ui_msgbox_info("专注完成", "25分钟专注已完成！");
    }

    focus_update_view();
}

static void focus_back_event_cb(lv_event_t *event)
{
    if(lv_event_get_code(event) == LV_EVENT_CLICKED) {
        lv_lib_pm_OpenPrePage(&page_manager);
    }
}

static void focus_toggle_event_cb(lv_event_t *event)
{
    if(lv_event_get_code(event) != LV_EVENT_CLICKED) {
        return;
    }

    if(focus_remaining_seconds == 0) {
        focus_remaining_seconds = FOCUS_DURATION_SECONDS;
    }

    focus_running = !focus_running;
    focus_update_view();
}

static void focus_reset_event_cb(lv_event_t *event)
{
    if(lv_event_get_code(event) == LV_EVENT_CLICKED) {
        focus_running = false;
        focus_remaining_seconds = FOCUS_DURATION_SECONDS;
        focus_update_view();
    }
}

static void focus_gesture_event_cb(lv_event_t *event)
{
    if(lv_event_get_code(event) != LV_EVENT_GESTURE) {
        return;
    }

    lv_dir_t direction = lv_indev_get_gesture_dir(lv_indev_get_act());
    if(direction == LV_DIR_LEFT || direction == LV_DIR_RIGHT) {
        lv_lib_pm_OpenPrePage(&page_manager);
    }
}

static lv_obj_t *focus_create_button(lv_obj_t *parent, const char *text, lv_coord_t x, lv_color_t color)
{
    lv_obj_t *button = lv_button_create(parent);
    lv_obj_set_size(button, 105, 46);
    lv_obj_set_pos(button, x, 172);
    lv_obj_set_style_radius(button, 14, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(button, color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(button, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *label = lv_label_create(button);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, &ui_font_focus22, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_center(label);

    return button;
}

void ui_FocusPage_init(void)
{
    focus_remaining_seconds = FOCUS_DURATION_SECONDS;
    focus_running = false;

    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x102A43), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(screen, focus_gesture_event_cb, LV_EVENT_GESTURE, NULL);

    lv_obj_t *back_button = lv_button_create(screen);
    lv_obj_set_size(back_button, 62, 34);
    lv_obj_set_pos(back_button, 12, 12);
    lv_obj_set_style_radius(back_button, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(back_button, lv_color_hex(0x334E68), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(back_button, focus_back_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *back_label = lv_label_create(back_button);
    lv_label_set_text(back_label, "返回");
    lv_obj_set_style_text_font(back_label, &ui_font_focus22, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_center(back_label);

    lv_obj_t *title_label = lv_label_create(screen);
    lv_label_set_text(title_label, "番茄专注");
    lv_obj_set_style_text_color(title_label, lv_color_hex(0xF0F4F8), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(title_label, &ui_font_focus22, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 15);

    focus_time_label = lv_label_create(screen);
    lv_obj_set_style_text_color(focus_time_label, lv_color_hex(0xF7B801), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(focus_time_label, &lv_font_montserrat_30, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(focus_time_label, LV_ALIGN_CENTER, 0, -18);

    focus_status_label = lv_label_create(screen);
    lv_obj_set_style_text_color(focus_status_label, lv_color_hex(0xD9E2EC), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(focus_status_label, &ui_font_focus22, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(focus_status_label, LV_ALIGN_CENTER, 0, 28);

    lv_obj_t *action_button = focus_create_button(screen, "开始", 48, lv_color_hex(0x2CB67D));
    focus_action_label = lv_obj_get_child(action_button, 0);
    lv_obj_add_event_cb(action_button, focus_toggle_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *reset_button = focus_create_button(screen, "重置", 167, lv_color_hex(0x486581));
    lv_obj_add_event_cb(reset_button, focus_reset_event_cb, LV_EVENT_CLICKED, NULL);

    focus_timer = lv_timer_create(focus_timer_cb, 1000, NULL);
    focus_update_view();
    lv_scr_load_anim(screen, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 100, 0, true);
}

void ui_FocusPage_deinit(void)
{
    if(focus_timer != NULL) {
        lv_timer_delete(focus_timer);
        focus_timer = NULL;
    }
}
