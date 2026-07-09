/**
 * @file capilot_bsp_i2c.h
 * @brief CaPilot BSP I2C 总线轻量接口
 *
 * 仅包含 I2C 总线初始化与通用读写 API，供不需要 LCD/触摸/LVGL 的组件
 * （如 capilot_imu、capilot_audio）复用，避免引入触摸/LVGL 头文件依赖。
 *
 * 完整板级 API 见 capilot_bsp.h。
 */
#pragma once

#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 不透明总线句柄（实际类型是 i2c_master_bus_handle_t，这里用 void* 隐藏实现） */
typedef void *capilot_bsp_i2c_bus_handle_t;

/* I2C 总线引脚与参数 */
#define CAPILOT_BSP_I2C_SDA         (GPIO_NUM_1)
#define CAPILOT_BSP_I2C_SCL         (GPIO_NUM_2)
#define CAPILOT_BSP_I2C_NUM         (0)
#define CAPILOT_BSP_I2C_FREQ_HZ     400000

/**
 * @brief 初始化 I2C 主机总线
 * @return ESP_OK 成功，否则失败
 */
esp_err_t capilot_bsp_i2c_init(void);

/**
 * @brief 获取 I2C 总线句柄（供其他组件创建自己的 device handle）
 * @return 总线句柄，未初始化返回 NULL
 */
capilot_bsp_i2c_bus_handle_t capilot_bsp_i2c_get_bus_handle(void);

/**
 * @brief 从 I2C 设备寄存器读取数据（写寄存器地址 + 读 N 字节）
 * @param dev_addr  7-bit I2C 设备地址
 * @param reg       寄存器地址
 * @param data      读取缓冲区
 * @param len       读取字节数
 * @return ESP_OK 成功
 * @note  必须在 capilot_bsp_i2c_init() 之后调用
 */
esp_err_t capilot_bsp_i2c_read(uint8_t dev_addr, uint8_t reg, uint8_t *data, size_t len);

/**
 * @brief 向 I2C 设备寄存器写入单字节
 * @param dev_addr  7-bit I2C 设备地址
 * @param reg       寄存器地址
 * @param val       写入字节
 * @return ESP_OK 成功
 * @note  必须在 capilot_bsp_i2c_init() 之后调用
 */
esp_err_t capilot_bsp_i2c_write_byte(uint8_t dev_addr, uint8_t reg, uint8_t val);

#ifdef __cplusplus
}
#endif