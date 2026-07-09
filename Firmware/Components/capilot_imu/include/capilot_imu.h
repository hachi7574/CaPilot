/**
 * @file capilot_imu.h
 * @brief CaPilot IMU 组件 — 公共 API（芯片无关）
 *
 * 使用方式：
 *   @code
 *   capilot_imu_init();                          // 默认配置
 *   // 或：
 *   capilot_imu_config_t cfg = CAPILOT_IMU_CONFIG_DEFAULT();
 *   cfg.acc_range = CAPILOT_IMU_ACC_RANGE_8G;
 *   capilot_imu_init_with_config(&cfg);
 *
 *   capilot_imu_data_t data;
 *   if (capilot_imu_read_with_angle(&data) == ESP_OK) {
 *       // data.acc_x, data.angle_x, ...
 *   }
 *   @endcode
 *
 * 换芯片：实现 capilot_imu_driver_t 接口，在 capilot_imu.c 中注册，
 *          无需修改本文件或调用方代码。
 */
#pragma once

#include "capilot_imu_types.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化 IMU（默认配置）
 * @note  自动探测已注册的驱动，找到第一个在线的芯片并初始化
 * @return ESP_OK 成功，ESP_ERR_NOT_FOUND 无芯片在线
 */
esp_err_t capilot_imu_init(void);

/**
 * @brief 初始化 IMU（指定配置）
 * @param[in] config 芯片无关配置（量程/ODR），NULL = 默认
 * @return ESP_OK 成功
 */
esp_err_t capilot_imu_init_with_config(const capilot_imu_config_t *config);

/**
 * @brief 读取加速度 + 陀螺仪原始值
 * @param[out] data 采样数据输出（acc/gyr 填充，angle 不变）
 * @return ESP_OK 成功，ESP_ERR_INVALID_STATE 数据未就绪，其他表示硬件错误
 * @note  若返回 ESP_ERR_INVALID_STATE，表示传感器数据尚未更新，调用方可重试
 */
esp_err_t capilot_imu_read(capilot_imu_data_t *data);

/**
 * @brief 读取加速度 + 陀螺仪，并计算三轴倾角（度）
 * @param[out] data 采样数据输出（acc/gyr + angle_x/y/z 全部填充）
 * @return ESP_OK 成功
 */
esp_err_t capilot_imu_read_with_angle(capilot_imu_data_t *data);

/**
 * @brief 检查是否有 IMU 芯片在线（任一已注册驱动响应 WHO_AM_I）
 * @return true 至少一片芯片在线
 */
bool capilot_imu_is_present(void);

/**
 * @brief 获取当前激活的驱动名称（用于日志/调试）
 * @return 驱动名称字符串（如 "qmi8658"），未初始化返回 "(none)"
 */
const char *capilot_imu_get_driver_name(void);

#ifdef __cplusplus
}
#endif
