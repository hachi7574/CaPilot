/**
 * @file capilot_imu_types.h
 * @brief CaPilot IMU 组件 — 芯片无关的数据类型与配置枚举
 *
 * 所有 IMU 芯片驱动共用这套类型定义。
 * 新芯片驱动只需实现 capilot_imu_driver_t vtable，无需修改本文件。
 *
 * 用量程枚举代替魔法数字，上层配置与芯片寄存器解耦。
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============ 加速度计量程 ============ */
typedef enum {
    CAPILOT_IMU_ACC_RANGE_2G  = 0,  /**< ±2g  */
    CAPILOT_IMU_ACC_RANGE_4G  = 1,  /**< ±4g  */
    CAPILOT_IMU_ACC_RANGE_8G  = 2,  /**< ±8g  */
    CAPILOT_IMU_ACC_RANGE_16G = 3,  /**< ±16g */
} capilot_imu_acc_range_t;

/* ============ 陀螺仪量程 ============ */
typedef enum {
    CAPILOT_IMU_GYR_RANGE_16DPS  = 0,  /**< ±16  dps  */
    CAPILOT_IMU_GYR_RANGE_32DPS  = 1,  /**< ±32  dps  */
    CAPILOT_IMU_GYR_RANGE_64DPS  = 2,  /**< ±64  dps  */
    CAPILOT_IMU_GYR_RANGE_128DPS = 3,  /**< ±128 dps  */
    CAPILOT_IMU_GYR_RANGE_256DPS = 4,  /**< ±256 dps  */
    CAPILOT_IMU_GYR_RANGE_512DPS = 5,  /**< ±512 dps  */
    CAPILOT_IMU_GYR_RANGE_1024DPS = 6, /**< ±1024 dps */
    CAPILOT_IMU_GYR_RANGE_2048DPS = 7, /**< ±2048 dps */
} capilot_imu_gyr_range_t;

/* ============ 输出数据率（ODR） ============ */
typedef enum {
    CAPILOT_IMU_ODR_125HZ  = 0,
    CAPILOT_IMU_ODR_250HZ  = 1,
    CAPILOT_IMU_ODR_500HZ  = 2,
    CAPILOT_IMU_ODR_1000HZ = 3,
    CAPILOT_IMU_ODR_2000HZ = 4,
    CAPILOT_IMU_ODR_4000HZ = 5,
} capilot_imu_odr_t;

/* ============ IMU 配置（芯片无关） ============ */
typedef struct {
    capilot_imu_acc_range_t acc_range;  /**< 加速度计量程 */
    capilot_imu_gyr_range_t gyr_range;  /**< 陀螺仪量程   */
    capilot_imu_odr_t      odr;       /**< 输出数据率   */
} capilot_imu_config_t;

/* ============ 默认配置（匹配 QMI8658 当前行为） ============ */
#define CAPILOT_IMU_CONFIG_DEFAULT() { \
    .acc_range = CAPILOT_IMU_ACC_RANGE_4G,  \
    .gyr_range = CAPILOT_IMU_GYR_RANGE_512DPS, \
    .odr       = CAPILOT_IMU_ODR_250HZ, \
}

/* ============ 原始采样数据 ============ */
typedef struct {
    int16_t acc_x;      /**< 加速度 X（芯片原始 LSB） */
    int16_t acc_y;      /**< 加速度 Y */
    int16_t acc_z;      /**< 加速度 Z */
    int16_t gyr_x;      /**< 陀螺仪 X（芯片原始 LSB） */
    int16_t gyr_y;      /**< 陀螺仪 Y */
    int16_t gyr_z;      /**< 陀螺仪 Z */
    float   angle_x;    /**< 俯仰角（度，由加速度计算） */
    float   angle_y;    /**< 横滚角（度，由加速度计算） */
    float   angle_z;    /**< 偏航角（度，由加速度计算） */
} capilot_imu_data_t;

/* ============ 量程 → 物理量转换系数 ============ */
/* 由具体芯片驱动提供，因为不同芯片的 LSB 定义不同 */

#ifdef __cplusplus
}
#endif
