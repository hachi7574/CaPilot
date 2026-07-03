/**
 * @file capilot_hid_keyboard.h
 * @brief CaPilot HID 键盘组件 - 高层 API
 *
 * 提供 BLE HID 键盘的简洁接口，封装底层 esp_hidd_send_keyboard_value()。
 * 调用流程：
 *   1. capilot_hid_init()              - 初始化 BLE HID
 *   2. capilot_hid_is_connected()      - 检查是否已配对连接
 *   3. capilot_hid_send_key(code)      - 发送按键（按下 + 抬起）
 *   4. capilot_hid_send_key_combo(...) - 发送组合键（如 Shift+3 = '#'）
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============ HID USB 键码（HID Usage Table 1.12） ============ */
/* 字母 A-Z */
#define HID_KEY_A           0x04
#define HID_KEY_B           0x05
#define HID_KEY_C           0x06
#define HID_KEY_D           0x07
#define HID_KEY_E           0x08
#define HID_KEY_F           0x09
#define HID_KEY_G           0x0A
#define HID_KEY_H           0x0B
#define HID_KEY_I           0x0C
#define HID_KEY_J           0x0D
#define HID_KEY_K           0x0E
#define HID_KEY_L           0x0F
#define HID_KEY_M           0x10
#define HID_KEY_N           0x11
#define HID_KEY_O           0x12
#define HID_KEY_P           0x13
#define HID_KEY_Q           0x14
#define HID_KEY_R           0x15
#define HID_KEY_S           0x16
#define HID_KEY_T           0x17
#define HID_KEY_U           0x18
#define HID_KEY_V           0x19
#define HID_KEY_W           0x1A
#define HID_KEY_X           0x1B
#define HID_KEY_Y           0x1C
#define HID_KEY_Z           0x1D

/* 数字 1-0（顶排） */
#define HID_KEY_1           0x1E
#define HID_KEY_2           0x1F
#define HID_KEY_3           0x20
#define HID_KEY_4           0x21
#define HID_KEY_5           0x22
#define HID_KEY_6           0x23
#define HID_KEY_7           0x24
#define HID_KEY_8           0x25
#define HID_KEY_9           0x26
#define HID_KEY_0           0x27

/* 控制键 */
#define HID_KEY_ENTER       0x28
#define HID_KEY_ESC         0x29
#define HID_KEY_BACKSPACE   0x2A
#define HID_KEY_TAB         0x2B
#define HID_KEY_SPACE       0x2C

/* 方向键 */
#define HID_KEY_RIGHT       0x4F
#define HID_KEY_LEFT        0x50
#define HID_KEY_DOWN        0x51
#define HID_KEY_UP          0x52

/* 修饰键掩码（来自 esp_hidd_prf_api.h） */
#define HID_MOD_LEFT_CTRL   (1 << 0)
#define HID_MOD_LEFT_SHIFT  (1 << 1)
#define HID_MOD_LEFT_ALT    (1 << 2)
#define HID_MOD_LEFT_GUI    (1 << 3)
#define HID_MOD_RIGHT_CTRL  (1 << 4)
#define HID_MOD_RIGHT_SHIFT (1 << 5)
#define HID_MOD_RIGHT_ALT   (1 << 6)
#define HID_MOD_RIGHT_GUI   (1 << 7)


/* ============ API ============ */

/**
 * @brief 初始化 BLE HID 键盘服务（启动蓝牙、广播、注册回调）
 * @return ESP_OK 或错误码
 */
esp_err_t capilot_hid_init(void);

/**
 * @brief 反初始化 HID 服务（关闭蓝牙）
 */
void capilot_hid_deinit(void);

/**
 * @brief 检查 BLE HID 是否已与主机配对连接
 * @return true 已连接，可发送按键；false 未连接
 */
bool capilot_hid_is_connected(void);

/**
 * @brief 发送单个按键（按下 + 抬起，自带 20ms 间隔）
 * @param key_code HID 键码，如 HID_KEY_ENTER、HID_KEY_5
 * @return ESP_OK 成功；ESP_ERR_INVALID_STATE 未连接；其他错误
 */
esp_err_t capilot_hid_send_key(uint8_t key_code);

/**
 * @brief 发送带修饰键的组合键（按下 + 抬起）
 * @param modifier 修饰键掩码，如 HID_MOD_LEFT_SHIFT
 * @param key_code HID 键码
 * @return ESP_OK 成功；ESP_ERR_INVALID_STATE 未连接
 *
 * 示例：
 *   capilot_hid_send_key_combo(HID_MOD_LEFT_SHIFT, HID_KEY_3);  // 发送 '#'
 *   capilot_hid_send_key_combo(HID_MOD_LEFT_SHIFT, HID_KEY_8);  // 发送 '*'
 */
esp_err_t capilot_hid_send_key_combo(uint8_t modifier, uint8_t key_code);

/**
 * @brief 按下按键（不抬起，用于长按场景）
 * @param key_code HID 键码
 * @return ESP_OK 或错误
 */
esp_err_t capilot_hid_press_key(uint8_t key_code);

/**
 * @brief 按下带修饰键的按键
 */
esp_err_t capilot_hid_press_key_combo(uint8_t modifier, uint8_t key_code);

/**
 * @brief 释放所有按键（发送空报告）
 * @return ESP_OK 或错误
 */
esp_err_t capilot_hid_release_all(void);

#ifdef __cplusplus
}
#endif
