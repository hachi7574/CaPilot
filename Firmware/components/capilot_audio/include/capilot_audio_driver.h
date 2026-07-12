/**
 * @file capilot_audio_driver.h
 * @brief CaPilot Audio 组件 — 芯片驱动 vtable 接口（内部）
 *
 * 新增音频芯片只需：
 *   1. 实现本文件定义的 capilot_audio_driver_t 接口
 *   2. 在 capilot_audio.c 的 s_drivers[] 中注册
 *   3. 无需修改公共 API（capilot_audio.h）
 *
 * 设计：函数指针 vtable，由具体芯片驱动填充，
 *       调度层通过 s_active_driver 指针调用。
 */

#pragma once

#include "capilot_audio_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============ 驱动 vtable ============ */
typedef struct capilot_audio_driver_t {
    const char *name;  /**< 驱动名称，用于日志（如 "es7210", "es8311"） */

    /**
     * @brief 初始化音频芯片
     * @param[in] config 芯片无关配置（具体驱动映射为寄存器值）
     * @return ESP_OK 成功，其他表示失败
     */
    esp_err_t (*init)(const capilot_audio_config_t *config);

    /**
     * @brief 配置音频芯片
     * @param[in] config 芯片无关配置
     * @return ESP_OK 成功，其他表示失败
     */
    esp_err_t (*config)(const capilot_audio_config_t *config);

    /**
     * @brief 开始采集/播放
     * @return ESP_OK 成功，其他表示失败
     */
    esp_err_t (*start)(void);

    /**
     * @brief 停止采集/播放
     * @return ESP_OK 成功，其他表示失败
     */
    esp_err_t (*stop)(void);

    /**
     * @brief 读取音频数据（采集）
     * @param[out] buffer 输出音频数据缓冲区
     * @return ESP_OK 成功，ESP_ERR_INVALID_STATE 数据未就绪，其他表示硬件错误
     * @note   调用方应处理 ESP_ERR_INVALID_STATE（非错误，可重试）
     */
    esp_err_t (*read)(capilot_audio_buffer_t *buffer);

    /**
     * @brief 写入音频数据（播放）
     * @param[in] buffer 输入音频数据缓冲区
     * @return ESP_OK 成功，ESP_ERR_INVALID_STATE 缓冲区满，其他表示硬件错误
     * @note   调用方应处理 ESP_ERR_INVALID_STATE（非错误，可重试）
     */
    esp_err_t (*write)(const capilot_audio_buffer_t *buffer);

    /**
     * @brief 检查芯片是否在线（读芯片 ID）
     * @return true 在线，false 离线或通信失败
     */
    bool (*is_present)(void);
} capilot_audio_driver_t;

#ifdef __cplusplus
}
#endif
