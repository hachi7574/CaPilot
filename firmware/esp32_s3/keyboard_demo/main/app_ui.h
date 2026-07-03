/**
 * @file app_ui.h
 * @brief 3×4 数字键盘 UI（0-9, *, #）
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 创建并显示 3×4 数字键盘界面
 *
 * 布局：
 *   [1] [2] [3]
 *   [4] [5] [6]
 *   [7] [8] [9]
 *   [*] [0] [#]
 *
 * 触摸任意按键即发送对应 HID 键码。
 */
void app_ui_create(void);

/**
 * @brief 更新状态栏（连接状态）
 * @param connected USB HID 是否已枚举成功
 */
void app_ui_update_status(bool connected);

#ifdef __cplusplus
}
#endif
