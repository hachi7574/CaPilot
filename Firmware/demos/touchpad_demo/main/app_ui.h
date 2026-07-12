/**
 * @file app_ui.h
 * @brief Touchpad Demo UI - 触摸板界面
 *
 * 全屏触摸板 + 顶部状态栏 + 底部操作提示。
 * 触摸事件通过 LVGL indev 回调传递给 main 的触摸板逻辑。
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 创建触摸板 UI（全屏触摸区域 + 状态栏 + 提示栏）
 */
void app_ui_create(void);

/**
 * @brief 更新状态栏连接状态
 */
void app_ui_update_status(bool connected);

/**
 * @brief 更新触摸板光标坐标显示（用于调试）
 * @param x 当前触摸 X 坐标
 * @param y 当前触摸 Y 坐标
 */
void app_ui_update_cursor(int x, int y);

#ifdef __cplusplus
}
#endif
