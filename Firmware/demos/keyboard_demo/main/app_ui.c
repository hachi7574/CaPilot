/**
 * @file app_ui.c
 * @brief 3×4 数字键盘 UI 实现
 *
 * 屏幕布局（320×240 横屏）：
 *   顶部 30px：状态栏（连接状态 + 标题）
 *   中间 180px：4 行 × 3 列 数字键盘按钮
 *   底部 30px：操作提示（"按 BOOT 键发送回车"）
 *
 * 触摸数字按钮即通过 capilot_hid_keyboard 发送对应键码。
 * '*' 和 '#' 是 Shift+8、Shift+3 组合键。
 */
#include "app_ui.h"
#include "capilot_usb_hid_keyboard.h"
#include "esp_log.h"
#include "lvgl.h"
#include "esp_lvgl_port.h"

static const char *TAG = "app_ui";

/* 状态栏标签 */
static lv_obj_t *status_label;
/* 标题 */
static lv_obj_t *title_label;
/* 提示标签 */
static lv_obj_t *hint_label;

/* 按键定义：标签 + HID 键码 + 是否需要 Shift */
typedef struct {
    const char *label;     /* 显示的文字 */
    uint8_t key_code;      /* HID 键码 */
    bool need_shift;       /* 是否需要 Shift 修饰键 */
} key_def_t;

/* 3×4 布局，按行排列：
 *   1 2 3
 *   4 5 6
 *   7 8 9
 *   * 0 #
 */
static const key_def_t key_map[4][3] = {
    { {"1", HID_KEY_1, false}, {"2", HID_KEY_2, false}, {"3", HID_KEY_3, false} },
    { {"4", HID_KEY_4, false}, {"5", HID_KEY_5, false}, {"6", HID_KEY_6, false} },
    { {"7", HID_KEY_7, false}, {"8", HID_KEY_8, false}, {"9", HID_KEY_9, false} },
    { {"*", HID_KEY_8, true }, {"0", HID_KEY_0, false}, {"#", HID_KEY_3, true } },
};

/* 按键事件回调 */
static void keypad_btn_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_CLICKED) {
        return;
    }

    lv_obj_t *btn = lv_event_get_target(e);
    const key_def_t *key = (const key_def_t *)lv_event_get_user_data(e);
    if (key == NULL) {
        return;
    }

    /* 视觉反馈：短暂改变按钮颜色 */
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x4CAF50), 0);
    /* 100ms 后恢复颜色（通过定时器实现） */
    /* 简化处理：用 lv_anim 或者直接延时恢复 - 这里直接靠 LVGL 默认状态切换 */

    if (!capilot_usb_hid_is_connected()) {
        ESP_LOGW(TAG, "USB HID not mounted, key '%s' ignored", key->label);
        lv_label_set_text_fmt(status_label, "#F44336 未连接# | 按键: %s (未发送)", key->label);
        return;
    }

    esp_err_t ret;
    if (key->need_shift) {
        ret = capilot_usb_hid_send_key_combo(HID_MOD_LEFT_SHIFT, key->key_code);
    } else {
        ret = capilot_usb_hid_send_key(key->key_code);
    }

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Sent key '%s' (HID code 0x%02X, shift=%d)",
                 key->label, key->key_code, key->need_shift);
        lv_label_set_text_fmt(status_label, "#4CAF50 已连接# | 已发送: %s", key->label);
    } else {
        ESP_LOGE(TAG, "Failed to send key '%s': %s", key->label, esp_err_to_name(ret));
        lv_label_set_text_fmt(status_label, "#F44336 错误# | %s", esp_err_to_name(ret));
    }
}

void app_ui_create(void)
{
    lvgl_port_lock(0);

    /* 设置屏幕背景色 */
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x1A1A1A), 0);
    lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, 0);

    /* === 顶部状态栏 (高度 30px) === */
    lv_obj_t *top_bar = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(top_bar);
    lv_obj_set_size(top_bar, 320, 30);
    lv_obj_align(top_bar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(top_bar, lv_color_hex(0x2C3E50), 0);
    lv_obj_set_style_bg_opa(top_bar, LV_OPA_COVER, 0);
    lv_obj_clear_flag(top_bar, LV_OBJ_FLAG_SCROLLABLE);

    title_label = lv_label_create(top_bar);
    lv_label_set_text(title_label, "CaPilot Keyboard");
    lv_obj_set_style_text_color(title_label, lv_color_hex(0xECF0F1), 0);
    lv_obj_align(title_label, LV_ALIGN_LEFT_MID, 5, 0);

    status_label = lv_label_create(top_bar);
    lv_label_set_text(status_label, "#F44336 未连接#");
    lv_label_set_recolor(status_label, true);
    lv_obj_set_style_text_color(status_label, lv_color_hex(0xECF0F1), 0);
    lv_obj_align(status_label, LV_ALIGN_RIGHT_MID, -5, 0);

    /* === 中间数字键盘区域 (高度 180px，y 偏移 30) === */
    lv_obj_t *keypad_area = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(keypad_area);
    lv_obj_set_size(keypad_area, 320, 180);
    lv_obj_align(keypad_area, LV_ALIGN_TOP_MID, 0, 30);
    lv_obj_clear_flag(keypad_area, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(keypad_area, 4, 0);
    lv_obj_set_style_pad_gap(keypad_area, 4, 0);
    lv_obj_set_flex_flow(keypad_area, LV_FLEX_FLOW_ROW_WRAP);

    /* 计算每个按钮的尺寸：4 行 3 列，间隔 4px，区域大小 312×172 */
    /* 按钮宽 = (312 - 2*4) / 3 = 101.3 -> 100 */
    /* 按钮高 = (172 - 3*4) / 4 = 40 */
    const int btn_w = 100;
    const int btn_h = 40;

    /* 创建 12 个按键 */
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 3; col++) {
            const key_def_t *key = &key_map[row][col];

            lv_obj_t *btn = lv_btn_create(keypad_area);
            lv_obj_set_size(btn, btn_w, btn_h);
            lv_obj_set_style_bg_color(btn, lv_color_hex(0x34495E), 0);
            lv_obj_set_style_bg_color(btn, lv_color_hex(0x4CAF50), LV_STATE_PRESSED);
            lv_obj_set_style_radius(btn, 6, 0);
            lv_obj_set_style_border_width(btn, 1, 0);
            lv_obj_set_style_border_color(btn, lv_color_hex(0x7F8C8D), 0);

            /* 注意：user_data 必须是稳定指针，使用 key_map 数组元素的地址 */
            lv_obj_add_event_cb(btn, keypad_btn_event_cb, LV_EVENT_ALL, (void *)key);

            lv_obj_t *label = lv_label_create(btn);
            lv_label_set_text(label, key->label);
            lv_obj_set_style_text_font(label, &lv_font_montserrat_20, 0);
            lv_obj_set_style_text_color(label, lv_color_hex(0xECF0F1), 0);
            lv_obj_center(label);
        }
    }

    /* === 底部提示栏 (高度 30px) === */
    lv_obj_t *bottom_bar = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(bottom_bar);
    lv_obj_set_size(bottom_bar, 320, 30);
    lv_obj_align(bottom_bar, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(bottom_bar, lv_color_hex(0x2C3E50), 0);
    lv_obj_set_style_bg_opa(bottom_bar, LV_OPA_COVER, 0);
    lv_obj_clear_flag(bottom_bar, LV_OBJ_FLAG_SCROLLABLE);

    hint_label = lv_label_create(bottom_bar);
    lv_label_set_text(hint_label, "BOOT = Enter  |  Touch keys to type");
    lv_obj_set_style_text_color(hint_label, lv_color_hex(0xBDC3C7), 0);
    lv_obj_align(hint_label, LV_ALIGN_CENTER, 0, 0);

    lvgl_port_unlock();

    ESP_LOGI(TAG, "Keypad UI created (3x4 grid: 0-9, *, #)");
}

void app_ui_update_status(bool connected)
{
    if (status_label == NULL) {
        return;
    }
    lvgl_port_lock(0);
    if (connected) {
        lv_label_set_text(status_label, "#4CAF50 已连接#");
    } else {
        lv_label_set_text(status_label, "#F44336 未连接#");
    }
    lvgl_port_unlock();
}
