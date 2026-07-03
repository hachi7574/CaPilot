/**
 * @file capilot_usb_hid_keyboard.h
 * @brief CaPilot USB HID 键盘组件 - 基于 TinyUSB
 *
 * 提供 USB HID 键盘的简洁接口，封装 TinyUSB 的 tud_hid_keyboard_report()。
 * 通过 USB-C 直连电脑，即插即用，无需配对。
 *
 * 调用流程：
 *   1. capilot_usb_hid_init()              - 初始化 USB HID
 *   2. capilot_usb_hid_is_connected()      - 检查 USB 是否已枚举成功
 *   3. capilot_usb_hid_send_key(code)      - 发送按键（按下 + 抬起）
 *   4. capilot_usb_hid_send_key_combo(...) - 发送组合键（如 Shift+3 = '#'）
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============ HID USB 键码（与 BLE 版本完全一致） ============ */
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

/* 修饰键掩码（TinyUSB 使用与 HID 标准一致的位掩码） */
#define HID_MOD_LEFT_CTRL   0x01
#define HID_MOD_LEFT_SHIFT  0x02
#define HID_MOD_LEFT_ALT    0x04
#define HID_MOD_LEFT_GUI    0x08
#define HID_MOD_RIGHT_CTRL  0x10
#define HID_MOD_RIGHT_SHIFT 0x20
#define HID_MOD_RIGHT_ALT   0x40
#define HID_MOD_RIGHT_GUI   0x80


/* ============ API ============ */

/**
 * @brief 初始化 USB HID 键盘服务（安装 TinyUSB 驱动）
 * @return ESP_OK 或错误码
 */
esp_err_t capilot_usb_hid_init(void);

/**
 * @brief 检查 USB HID 是否已枚举成功（电脑识别为键盘）
 * @return true 已连接可发送；false USB 未就绪
 */
bool capilot_usb_hid_is_connected(void);

/**
 * @brief 发送单个按键（按下 + 抬起，自带 20ms 间隔）
 * @param key_code HID 键码
 * @return ESP_OK 或错误
 */
esp_err_t capilot_usb_hid_send_key(uint8_t key_code);

/**
 * @brief 发送带修饰键的组合键（按下 + 抬起）
 * @param modifier 修饰键掩码
 * @param key_code HID 键码
 */
esp_err_t capilot_usb_hid_send_key_combo(uint8_t modifier, uint8_t key_code);

/**
 * @brief 按下按键（不抬起，用于长按场景）
 */
esp_err_t capilot_usb_hid_press_key_combo(uint8_t modifier, uint8_t key_code);

/**
 * @brief 释放所有按键
 */
esp_err_t capilot_usb_hid_release_all(void);

#ifdef __cplusplus
}
#endif
