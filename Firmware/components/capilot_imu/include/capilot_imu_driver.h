/**
 * @file capilot_imu_driver.h
 * @brief CaPilot IMU 组件 — 芯片驱动 vtable 接口（内部）
 *
 * 新增 IMU 芯片只需：
 *   1. 实现本文件定义的 capilot_imu_driver_t 接口
 *   2. 在 capilot_imu.c 的 s_drivers[] 中注册
 *   3. 无需修改公共 API（capilot_imu.h）
 *
 * 设计：函数指针 vtable，由具体芯片驱动填充，
 *       调度层通过 s_active_driver 指针调用。
 */
#pragma once

#include "capilot_imu_types.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============ 驱动 vtable ============ */
typedef struct capilot_imu_driver_t {
    const char *name;  /**< 驱动名称，用于日志（如 "qmi8658"） */

    /**
     * @brief 初始化 IMU 芯片
     * @param[in] config 芯片无关配置（具体驱动映射为寄存器值）
     * @return ESP_OK 成功，其他表示失败
     */
    esp_err_t (*init)(const capilot_imu_config_t *config);

    /**
     * @brief 读取加速度 + 陀螺仪原始值
     * @param[out] data 输出数据（acc/gyr 填充，angle 不变）
     * @return ESP_OK 成功，ESP_ERR_INVALID_STATE 数据未就绪，其他表示硬件错误
     * @note   调用方应处理 ESP_ERR_INVALID_STATE（非错误，可重试）
     */
    esp_err_t (*read)(capilot_imu_data_t *data);

    /**
     * @brief 读取原始值并计算三轴倾角
     * @param[out] data 输出数据（acc/gyr/angle 全部填充）
     * @return ESP_OK 成功
     */
    esp_err_t (*read_with_angle)(capilot_imu_data_t *data);

    /**
     * @brief 检查芯片是否在线（读 WHO_AM_I / 芯片 ID）
     * @return true 在线，false 离线或通信失败
     */
    bool (*is_present)(void);
} capilot_imu_driver_t;

#ifdef __cplusplus
}
#endif
