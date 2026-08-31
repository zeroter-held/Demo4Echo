#include "ui_FocusPage.h"

#include <stdio.h>
#include <stdlib.h>

#define DEFAULT_WORK_SECONDS (25 * 60)
#define BREAK_SECONDS (5 * 60)
#define MIN_WORK_SECONDS (5 * 60)
#define MAX_WORK_SECONDS (120 * 60)
#define STEP_SECONDS (5 * 60)
#define COUNT_FILE "/root/bin/focus_count.txt"

/* true=休息模式, false=工作模式 */
static bool focus_is_break;
static uint32_t focus_remaining_seconds;
static bool focus_running;
static uint32_t focus_work_seconds;
static uint32_t focus_today_count;
static lv_timer_t *focus_timer;
static lv_obj_t *focus_time_label;
static lv_obj_t *focus_status_label;
static lv_obj_t *focus_action_label;
static lv_obj_t *focus_count_label;
static lv_obj_t *focus_duration_label;

static void focus_load_count(void)
{
    FILE *fp = fopen(COUNT_FILE, "r");
    int saved_day = 0;
    unsigned int saved_count = 0;

    if(fp) {
        if(fscanf(fp, "%d %u", &saved_day, &saved_count) == 2) {
            int y, m, d, h, mi, s;
            sys_get_time(&y, &m, &d, &h, &mi, &s);
            int today = y * 10000 + m * 100 + d;
            focus_today_count = (saved_day == today) ? saved_count : 0;
        } else {
            focus_today_count = 0;
        }
        fclose(fp);
    } else {
        focus_today_count = 0;
    }
}

static void focus_save_count(void)
{
    int y, m, d, h, mi, s;
    sys_get_time(&y, &m, &d, &h, &mi, &s);
    int today = y * 10000 + m * 100 + d;

    FILE *fp = fopen(COUNT_FILE, "w");
    if(fp) {
        fprintf(fp, "%d %u\n", today, focus_today_count);
        fclose(fp);
    }
}

static void focus_update_view(void)
{
    char text[32];
    uint32_t minutes = focus_remaining_seconds / 60;
    uint32_t seconds = focus_remaining_seconds % 60;

    snprintf(text, sizeof(text), "%02lu:%02lu", (unsigned long)minutes, (unsigned long)seconds);
    lv_label_set_text(focus_time_label, text);

    if(focus_running) {
        lv_label_set_text(focus_status_label, focus_is_break ? "休息中" : "专注进行中");
        lv_label_set_text(focus_action_label, "暂停");
    } else if(focus_remaining_seconds == 0) {
        lv_label_set_text(focus_status_label, focus_is_break ? "休息完成" : "本轮专注完成");
        lv_label_set_text(focus_action_label, "再开始");
    } else {
        lv_label_set_text(focus_status_label, focus_is_break ? "准备休息" : "准备开始");
        lv_label_set_text(focus_action_label, "开始");
    }

    snprintf(text, sizeof(text), "%lu分钟", (unsigned long)(focus_work_seconds / 60));
    lv_label_set_text(focus_duration_label, text);

    snprintf(text, sizeof(text), "今日%lu", (unsigned long)focus_today_count);
    lv_label_set_text(focus_count_label, text);
}

static void focus_play_done_sound(void)
{
    system("aplay /root/bin/third_party/audio/focus_done.wav > /dev/null 2>&1 &");
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

        if(focus_is_break) {
            /* 休息结束，切回工作 */
            focus_is_break = false;
            focus_remaining_seconds = focus_work_seconds;
            ui_msgbox_info("休息完成", "5分钟休息结束，开始下一轮专注吧");
        } else {
            /* 工作结束，进入休息，计数+1 */
            focus_is_break = true;
            focus_remaining_seconds = BREAK_SECONDS;
            focus_today_count++;
            focus_save_count();
            ui_msgbox_info("专注完成", "本轮专注完成，休息5分钟");
        }
        focus_play_done_sound();
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
        focus_remaining_seconds = focus_is_break ? BREAK_SECONDS : focus_work_seconds;
    }

    focus_running = !focus_running;
    focus_update_view();
}

static void focus_reset_event_cb(lv_event_t *event)
{
    if(lv_event_get_code(event) == LV_EVENT_CLICKED) {
        focus_running = false;
        focus_is_break = false;
        focus_remaining_seconds = focus_work_seconds;
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

static void focus_duration_dec_event_cb(lv_event_t *event)
{
    if(lv_event_get_code(event) != LV_EVENT_CLICKED) {
        return;
    }

    /* 运行中不允许调整时长 */
    if(focus_running) {
        return;
    }

    if(focus_work_seconds > MIN_WORK_SECONDS) {
        focus_work_seconds -= STEP_SECONDS;
        if(!focus_is_break) {
            focus_remaining_seconds = focus_work_seconds;
        }
        focus_update_view();
    }
}

static void focus_duration_inc_event_cb(lv_event_t *event)
{
    if(lv_event_get_code(event) != LV_EVENT_CLICKED) {
        return;
    }

    if(focus_running) {
        return;
    }

    if(focus_work_seconds < MAX_WORK_SECONDS) {
        focus_work_seconds += STEP_SECONDS;
        if(!focus_is_break) {
            focus_remaining_seconds = focus_work_seconds;
        }
        focus_update_view();
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

static lv_obj_t *focus_create_small_button(lv_obj_t *parent, const char *text, lv_coord_t x, lv_coord_t y)
{
    lv_obj_t *button = lv_button_create(parent);
    lv_obj_set_size(button, 40, 30);
    lv_obj_set_pos(button, x, y);
    lv_obj_set_style_radius(button, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(button, lv_color_hex(0x334E68), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(button, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *label = lv_label_create(button);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, &ui_font_focus22, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_center(label);

    return button;
}

void ui_FocusPage_init(void)
{
    focus_is_break = false;
    focus_work_seconds = DEFAULT_WORK_SECONDS;
    focus_remaining_seconds = focus_work_seconds;
    focus_running = false;
    focus_load_count();

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

    focus_count_label = lv_label_create(screen);
    lv_obj_set_style_text_color(focus_count_label, lv_color_hex(0xD9E2EC), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(focus_count_label, &ui_font_focus22, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(focus_count_label, LV_ALIGN_TOP_RIGHT, -12, 18);

    focus_time_label = lv_label_create(screen);
    lv_obj_set_style_text_color(focus_time_label, lv_color_hex(0xF7B801), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(focus_time_label, &lv_font_montserrat_30, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(focus_time_label, LV_ALIGN_CENTER, 0, -45);

    focus_status_label = lv_label_create(screen);
    lv_obj_set_style_text_color(focus_status_label, lv_color_hex(0xD9E2EC), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(focus_status_label, &ui_font_focus22, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(focus_status_label, LV_ALIGN_CENTER, 0, -10);

    /* 时长调节行: [-] 分钟 [+] */
    lv_obj_t *minus_button = focus_create_small_button(screen, "-", 80, 138);
    lv_obj_add_event_cb(minus_button, focus_duration_dec_event_cb, LV_EVENT_CLICKED, NULL);

    focus_duration_label = lv_label_create(screen);
    lv_obj_set_style_text_color(focus_duration_label, lv_color_hex(0xF0F4F8), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(focus_duration_label, &ui_font_focus22, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(focus_duration_label, LV_ALIGN_CENTER, 0, 22);

    lv_obj_t *plus_button = focus_create_small_button(screen, "+", 200, 138);
    lv_obj_add_event_cb(plus_button, focus_duration_inc_event_cb, LV_EVENT_CLICKED, NULL);

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
