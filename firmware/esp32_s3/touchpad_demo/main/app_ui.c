/**
 * @file app_ui.c
 * @brief Touchpad Demo UI 实现
 *
 * 屏幕布局（320x240 横屏）：
 *   顶部 30px：状态栏（标题 + USB 连接状态）
 *   中间 180px：触摸板区域（全区域接收触摸）
 *   底部 30px：操作提示
 *
 * 触摸映射逻辑（类笔记本触摸板）：
 *   单指滑动      → 鼠标移动（不按按键）
 *   双击（2次快速点击） → 鼠标左键单击
 *   三击（3次快速点击） → 鼠标左键双击
 *
 * 判定规则：
 *   - 按下后移动超过 TAP_MOVE_THRESHOLD 像素 → 视为滑动，不计入点击
 *   - 两次释放间隔 < TAP_TIMEOUT_MS → 计入同一组点击
 *   - 超时后根据点击次数触发对应动作
 *   - 三击立即触发（不等超时），避免左键双击延迟过大
 */
#include "app_ui.h"
#include "capilot_usb_hid_mouse.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "esp_lvgl_port.h"

static const char *TAG = "app_ui";

/* 灵敏度系数：触摸位移 x 此系数 = 鼠标位移 */
#define TOUCH_SENSITIVITY   2

/* 单次 HID 报告最大位移（int8_t 范围） */
#define MAX_HID_MOVE        127

/* 双击/三击超时（ms）：两次释放间隔超过此值则视为新一轮点击 */
#define TAP_TIMEOUT_MS      400

/* 滑动判定阈值（像素）：按下后移动超过此距离则不算点击 */
#define TAP_MOVE_THRESHOLD  8

/* 左键双击间隔（ms） */
#define DOUBLE_CLICK_GAP_MS 30

/* UI 元素 */
static lv_obj_t *status_label;
static lv_obj_t *coord_label;
static lv_obj_t *action_label;
static lv_obj_t *touchpad_area;

/* 上次触摸坐标（用于计算相对位移） */
static int16_t last_x = 0;
static int16_t last_y = 0;

/* 按下时的起点坐标（用于判定滑动 vs 点击） */
static int16_t press_start_x = 0;
static int16_t press_start_y = 0;

/* 按下后是否已移动超过阈值 */
static bool has_moved = false;

/* 当前点击计数（1=首次, 2=双击, 3=三击） */
static int tap_count = 0;

/* 点击超时定时器 */
static esp_timer_handle_t tap_timer = NULL;

/* ============ 内部函数 ============ */

/* 将位移 clamp 到 int8_t 范围 */
static int8_t clamp_move(int v)
{
    if (v > MAX_HID_MOVE) return MAX_HID_MOVE;
    if (v < -MAX_HID_MOVE) return -MAX_HID_MOVE;
    return (int8_t)v;
}

/* 点击超时定时器回调（在 esp_timer 任务中执行，不持有 LVGL 锁） */
static void tap_timer_cb(void *arg)
{
    int count = tap_count;
    tap_count = 0;

    if (count == 2) {
        /* 双击 → 左键单击 */
        if (capilot_usb_hid_mouse_is_connected()) {
            capilot_usb_hid_mouse_click(CAPOLOT_MOUSE_BTN_LEFT);
        }
        ESP_LOGI(TAG, "Double tap -> Left click");
    } else if (count >= 3) {
        /* 三击 → 左键双击 */
        if (capilot_usb_hid_mouse_is_connected()) {
            capilot_usb_hid_mouse_click(CAPOLOT_MOUSE_BTN_LEFT);
            vTaskDelay(pdMS_TO_TICKS(DOUBLE_CLICK_GAP_MS));
            capilot_usb_hid_mouse_click(CAPOLOT_MOUSE_BTN_LEFT);
        }
        ESP_LOGI(TAG, "Triple tap -> Left double click");
    }
    /* count == 1 → 无动作 */
}

/* 启动/重启超时定时器 */
static void start_tap_timer(uint64_t timeout_us)
{
    if (tap_timer == NULL) return;
    esp_timer_stop(tap_timer);
    esp_timer_start_once(tap_timer, timeout_us);
}

/* ============ 触摸板事件回调 ============ */

static void touchpad_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    /* LVGL 8.x filter 是 == 比较，不支持 OR，用 LV_EVENT_ALL 后内部判断 */
    if (code != LV_EVENT_PRESSED && code != LV_EVENT_PRESSING && code != LV_EVENT_RELEASED) {
        return;
    }

    lv_indev_t *indev = lv_indev_get_act();
    if (indev == NULL) {
        return;
    }

    lv_point_t point;
    lv_indev_get_point(indev, &point);

    switch (code) {
    case LV_EVENT_PRESSED: {
        /* 触摸按下：记录起点，取消 pending 超时（可能构成多击） */
        last_x = point.x;
        last_y = point.y;
        press_start_x = point.x;
        press_start_y = point.y;
        has_moved = false;
        if (tap_timer) esp_timer_stop(tap_timer);

        app_ui_update_cursor(point.x, point.y);
        ESP_LOGI(TAG, "TOUCH PRESSED at (%d,%d)", point.x, point.y);
        break;
    }
    case LV_EVENT_PRESSING: {
        /* 触摸移动：纯鼠标移动（buttons=0），不按任何键 */
        int dx = (point.x - last_x) * TOUCH_SENSITIVITY;
        int dy = (point.y - last_y) * TOUCH_SENSITIVITY;

        if (dx != 0 || dy != 0) {
            /* 只有 HID 端点就绪才发送，否则保留 last_x/last_y 让位移累加 */
            int8_t clamped_dx = clamp_move(dx);
            int8_t clamped_dy = clamp_move(dy);
            bool sent = false;

            if (capilot_usb_hid_mouse_ready()) {
                esp_err_t ret = capilot_usb_hid_mouse_report(0, clamped_dx, clamped_dy, 0);
                sent = (ret == ESP_OK);
            }

            if (sent) {
                last_x = point.x;
                last_y = point.y;
                ESP_LOGI(TAG, "MOVE raw(%d,%d) HID(%d,%d) OK",
                         dx, dy, clamped_dx, clamped_dy);
            } else {
                ESP_LOGD(TAG, "MOVE raw(%d,%d) SKIPPED (not ready)", dx, dy);
            }

            /* 检查移动距离是否超过滑动阈值 */
            int16_t move_dx = point.x - press_start_x;
            int16_t move_dy = point.y - press_start_y;
            if (move_dx < 0) move_dx = -move_dx;
            if (move_dy < 0) move_dy = -move_dy;
            if (move_dx > TAP_MOVE_THRESHOLD || move_dy > TAP_MOVE_THRESHOLD) {
                has_moved = true;
            }
        }
        app_ui_update_cursor(point.x, point.y);
        break;
    }
    case LV_EVENT_RELEASED: {
        /* 触摸抬起：判断是滑动还是点击 */
        if (has_moved) {
            /* 滑动 → 不计点击 */
            tap_count = 0;
            ESP_LOGI(TAG, "TOUCH RELEASED (swipe)");
        } else {
            /* 点击 → 计数+1 */
            tap_count++;
            if (tap_count >= 3) {
                /* 三击 → 1ms 后立即触发左键双击（不等超时） */
                start_tap_timer(1 * 1000);
            } else {
                /* 1击或2击 → 等超时确认 */
                start_tap_timer((uint64_t)TAP_TIMEOUT_MS * 1000);
            }
            ESP_LOGI(TAG, "TOUCH TAP #%d (will trigger on timeout)", tap_count);
        }
        break;
    }
    default:
        break;
    }
}

/* ============ UI 创建 ============ */

void app_ui_create(void)
{
    lvgl_port_lock(0);

    /* 屏幕背景色 */
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x1A1A1A), 0);
    lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, 0);

    /* === 顶部状态栏 (高 30px) === */
    lv_obj_t *top_bar = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(top_bar);
    lv_obj_set_size(top_bar, 320, 30);
    lv_obj_align(top_bar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(top_bar, lv_color_hex(0x2C3E50), 0);
    lv_obj_set_style_bg_opa(top_bar, LV_OPA_COVER, 0);
    lv_obj_clear_flag(top_bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title_label = lv_label_create(top_bar);
    lv_label_set_text(title_label, "CaPilot Touchpad");
    lv_obj_set_style_text_color(title_label, lv_color_hex(0xECF0F1), 0);
    lv_obj_align(title_label, LV_ALIGN_LEFT_MID, 5, 0);

    status_label = lv_label_create(top_bar);
    lv_label_set_text(status_label, "#F44336 未连接#");
    lv_label_set_recolor(status_label, true);
    lv_obj_set_style_text_color(status_label, lv_color_hex(0xECF0F1), 0);
    lv_obj_align(status_label, LV_ALIGN_RIGHT_MID, -5, 0);

    /* === 中间触摸板区域 (高 180px, y=30) === */
    touchpad_area = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(touchpad_area);
    lv_obj_set_size(touchpad_area, 320, 180);
    lv_obj_align(touchpad_area, LV_ALIGN_TOP_MID, 0, 30);
    lv_obj_set_style_bg_color(touchpad_area, lv_color_hex(0x34495E), 0);
    lv_obj_set_style_bg_opa(touchpad_area, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(touchpad_area, 2, 0);
    lv_obj_set_style_border_color(touchpad_area, lv_color_hex(0x4CAF50), 0);
    lv_obj_set_style_radius(touchpad_area, 8, 0);
    lv_obj_clear_flag(touchpad_area, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(touchpad_area, LV_OBJ_FLAG_CLICKABLE);

    /* PRESSED 状态视觉反馈 */
    lv_obj_set_style_bg_color(touchpad_area, lv_color_hex(0x4CAF50), LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(touchpad_area, LV_OPA_60, LV_STATE_PRESSED);

    /* 触摸板提示文字 */
    lv_obj_t *hint = lv_label_create(touchpad_area);
    lv_label_set_text(hint, "Drag = Move\nDouble-tap = Left Click\nTriple-tap = Left Dbl-Click");
    lv_obj_set_style_text_color(hint, lv_color_hex(0xBDC3C7), 0);
    lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(hint);

    /* 坐标显示（调试用，右上角） */
    coord_label = lv_label_create(touchpad_area);
    lv_label_set_text(coord_label, "(--, --)");
    lv_obj_set_style_text_color(coord_label, lv_color_hex(0xF39C12), 0);
    lv_obj_align(coord_label, LV_ALIGN_TOP_RIGHT, -5, 5);

    /* 注册触摸板事件回调（LVGL 8.x filter 不支持 OR，用 LV_EVENT_ALL） */
    lv_obj_add_event_cb(touchpad_area, touchpad_event_cb,
                        LV_EVENT_ALL, NULL);

    /* === 底部提示栏 (高 30px) === */
    lv_obj_t *bottom_bar = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(bottom_bar);
    lv_obj_set_size(bottom_bar, 320, 30);
    lv_obj_align(bottom_bar, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(bottom_bar, lv_color_hex(0x2C3E50), 0);
    lv_obj_set_style_bg_opa(bottom_bar, LV_OPA_COVER, 0);
    lv_obj_clear_flag(bottom_bar, LV_OBJ_FLAG_SCROLLABLE);

    action_label = lv_label_create(bottom_bar);
    lv_label_set_text(action_label, "BOOT = Right Click  |  Drag = Move  |  2x = L-Click  |  3x = L-Dbl");
    lv_obj_set_style_text_color(action_label, lv_color_hex(0xBDC3C7), 0);
    lv_obj_align(action_label, LV_ALIGN_CENTER, 0, 0);

    lvgl_port_unlock();

    /* 创建点击超时定时器 */
    esp_timer_create_args_t timer_args = {
        .callback = tap_timer_cb,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "tap_timer",
    };
    esp_timer_create(&timer_args, &tap_timer);

    ESP_LOGI(TAG, "Touchpad UI created (320x180, sensitivity=%d, tap_timeout=%dms)",
             TOUCH_SENSITIVITY, TAP_TIMEOUT_MS);
}

void app_ui_update_status(bool connected)
{
    if (status_label == NULL) return;
    lvgl_port_lock(0);
    if (connected) {
        lv_label_set_text(status_label, "#4CAF50 已连接#");
    } else {
        lv_label_set_text(status_label, "#F44336 未连接#");
    }
    lvgl_port_unlock();
}

void app_ui_update_cursor(int x, int y)
{
    if (coord_label == NULL) return;
    lvgl_port_lock(0);
    lv_label_set_text_fmt(coord_label, "(%d, %d)", x, y);
    lvgl_port_unlock();
}
