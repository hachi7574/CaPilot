/**
 * @file capilot_audio_events.h
 * @brief CaPilot Audio 组件 — 音频事件 API
 *
 * 支持的事件类型：
 *   - VAD（语音活动检测）
 *   - Push To Talk（按键说话）
 *   - Typeless 语音输入
 *
 * 使用方式：
 *   @code
 *   capilot_audio_events_init();
 *   
 *   // 注册事件回调
 *   capilot_audio_events_register_callback(my_event_handler);
 *   
 *   // 启用 VAD
 *   capilot_audio_events_enable_vad(true);
 *   @endcode
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============ 音频事件类型 ============ */
typedef enum {
    CAPILOT_AUDIO_EVENT_VAD_START   = 0,  /**< 语音活动开始 */
    CAPILOT_AUDIO_EVENT_VAD_END     = 1,  /**< 语音活动结束 */
    CAPILOT_AUDIO_EVENT_PUSH_TALK  = 2,  /**< Push To Talk 触发 */
    CAPILOT_AUDIO_EVENT_TYPELESS   = 3,  /**< Typeless 语音输入 */
} capilot_audio_event_type_t;

/* ============ 事件回调函数类型 ============ */
typedef void (*capilot_audio_event_callback_t)(capilot_audio_event_type_t event, void *user_data);

/**
 * @brief 初始化音频事件处理
 * @return ESP_OK 成功，其他表示失败
 */
esp_err_t capilot_audio_events_init(void);

/**
 * @brief 注册音频事件回调
 * @param[in] callback 回调函数指针
 * @return ESP_OK 成功，其他表示失败
 */
esp_err_t capilot_audio_events_register_callback(capilot_audio_event_callback_t callback);

/**
 * @brief 启用/禁用 VAD（语音活动检测）
 * @param[in] enable true=启用，false=禁用
 * @return ESP_OK 成功，其他表示失败
 */
esp_err_t capilot_audio_events_enable_vad(bool enable);

/**
 * @brief 启用/禁用 Push To Talk
 * @param[in] enable true=启用，false=禁用
 * @param[in] button_gpio Push To Talk 按钮 GPIO 编号
 * @return ESP_OK 成功，其他表示失败
 */
esp_err_t capilot_audio_events_enable_push_talk(bool enable, uint32_t button_gpio);

#ifdef __cplusplus
}
#endif
