/**
 * @file capilot_usb_hid_mouse.h
 * @brief CaPilot USB HID 鼠标组件 - 基于 TinyUSB
 *
 * 通过 USB-C 直连电脑，模拟标准 HID 鼠标。
 * 支持三键（左/右/中）+ X/Y 相对位移 + 滚轮。
 *
 * 调用流程：
 *   1. capilot_usb_hid_mouse_init()           - 初始化 USB HID 鼠标
 *   2. capilot_usb_hid_mouse_is_connected()   - 检查 USB 是否已枚举
 *   3. capilot_usb_hid_mouse_report(...)      - 发送鼠标报告（按键状态 + 位移 + 滚轮）
 *   4. capilot_usb_hid_mouse_move(dx, dy)     - 仅移动（便捷封装）
 *   5. capilot_usb_hid_mouse_click(btn)       - 单击（按下+抬起，便捷封装）
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============ 鼠标按键掩码 ============ */
/* 注意：加 CAPOLOT_ 前缀避免与 TinyUSB hid.h 的 MOUSE_BUTTON_LEFT 枚举冲突 */
#define CAPOLOT_MOUSE_BTN_LEFT     0x01
#define CAPOLOT_MOUSE_BTN_RIGHT    0x02
#define CAPOLOT_MOUSE_BTN_MIDDLE   0x04

/* ============ API ============ */

/**
 * @brief 初始化 USB HID 鼠标服务（安装 TinyUSB 驱动）
 * @return ESP_OK 或错误码
 */
esp_err_t capilot_usb_hid_mouse_init(void);

/**
 * @brief 检查 USB HID 是否已枚举成功（电脑识别为鼠标）
 * @return true 已连接可发送；false USB 未就绪
 */
bool capilot_usb_hid_mouse_is_connected(void);

/**
 * @brief 检查 HID 端点是否就绪（可立即发送报告）
 * @return true 端点就绪；false 上一个报告尚未发送完
 *
 * @note 连续发送鼠标移动前应先调用此函数，避免报告被丢弃。
 *       is_connected 只表示 USB 已枚举，不代表端点可写。
 */
bool capilot_usb_hid_mouse_ready(void);

/**
 * @brief 发送鼠标报告
 * @param buttons  按键位掩码（MOUSE_BUTTON_LEFT | MOUSE_BUTTON_RIGHT | MOUSE_BUTTON_MIDDLE）
 * @param dx       X 轴相对位移（-127 ~ 127）
 * @param dy       Y 轴相对位移（-127 ~ 127）
 * @param wheel    滚轮位移（-127 ~ 127，0 = 不滚动）
 * @return ESP_OK 或错误
 *
 * @note 每次调用发送一帧报告。要持续移动需持续调用。
 *       位移为 0 时只更新按键状态，不移动光标。
 */
esp_err_t capilot_usb_hid_mouse_report(uint8_t buttons, int8_t dx, int8_t dy, int8_t wheel);

/**
 * @brief 仅移动鼠标（便捷封装，不改变按键状态）
 * @param dx X 轴相对位移
 * @param dy Y 轴相对位移
 */
esp_err_t capilot_usb_hid_mouse_move(int8_t dx, int8_t dy);

/**
 * @brief 单击鼠标按键（按下 + 抬起，自带 20ms 间隔）
 * @param button MOUSE_BUTTON_LEFT / RIGHT / MIDDLE
 */
esp_err_t capilot_usb_hid_mouse_click(uint8_t button);

/**
 * @brief 按下鼠标按键（不抬起）
 * @param button 按键位掩码
 */
esp_err_t capilot_usb_hid_mouse_press(uint8_t button);

/**
 * @brief 释放所有鼠标按键
 */
esp_err_t capilot_usb_hid_mouse_release(void);

#ifdef __cplusplus
}
#endif
