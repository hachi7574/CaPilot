/**
 * @file capilot_audio.h
 * @brief CaPilot Audio 组件 — 公共 API（硬件无关）
 *
 * 使用方式：
 *   @code
 *   capilot_audio_init();                          // 默认配置
 *   // 或：
 *   capilot_audio_config_t cfg = CAPILOT_AUDIO_CONFIG_DEFAULT();
 *   capilot_audio_init_with_config(&cfg);
 *
 *   // 音频采集
 *   capilot_audio_capture_start();
 *
 *   // 音频播放
 *   capilot_audio_playback_start();
 *   @endcode
 *
 * 换芯片：实现 capilot_audio_driver_t 接口，在 capilot_audio.c 中注册，
 *          无需修改本文件或调用方代码。
 */

#pragma once

#include "capilot_audio_types.h"
#include "capilot_audio_capture.h"
#include "capilot_audio_playback.h"
#include "capilot_audio_events.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化 Audio 组件（默认配置）
 * @note  自动探测已注册的驱动，找到第一个在线的芯片并初始化
 * @return ESP_OK 成功，ESP_ERR_NOT_FOUND 无芯片在线
 */
esp_err_t capilot_audio_init(void);

/**
 * @brief 初始化 Audio 组件（指定配置）
 * @param[in] config 芯片无关配置，NULL = 默认
 * @return ESP_OK 成功
 */
esp_err_t capilot_audio_init_with_config(const capilot_audio_config_t *config);

/**
 * @brief 检查是否有音频芯片在线（任一已注册驱动响应）
 * @return true 至少一片芯片在线
 */
bool capilot_audio_is_present(void);

/**
 * @brief 获取当前激活的驱动名称（用于日志/调试）
 * @return 驱动名称字符串（如 "es7210", "es8311"），未初始化返回 "(none)"
 */
const char *capilot_audio_get_driver_name(void);

#ifdef __cplusplus
}
#endif
