#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* === 公共常量 ======================================================== */

#define {{COMPONENT_NAME_UPPER}}_I2C_ADDR   0x00   /* TODO: 修改 */

/* === 类型定义 ======================================================== */

/**
 * @brief {{COMPONENT_NAME}} 配置结构体
 */
typedef struct {
    /* TODO: 添加配置项 */
    uint8_t     i2c_addr;
    uint16_t    odr_hz;
} {{component_name}}_config_t;

/* === 默认配置 ======================================================== */

#define {{COMPONENT_NAME_UPPER}}_DEFAULT_CONFIG() { \
    .i2c_addr = {{COMPONENT_NAME_UPPER}}_I2C_ADDR,  \
    .odr_hz   = 100,                                 \
}

/* === API 函数声明 ==================================================== */

/**
 * @brief 初始化组件
 * @param cfg 配置参数，传 NULL 使用默认值
 * @return ESP_OK 成功
 */
esp_err_t {{component_name}}_init(const {{component_name}}_config_t *cfg);

/**
 * @brief 反初始化，释放资源
 * @return ESP_OK 成功
 */
esp_err_t {{component_name}}_deinit(void);

/**
 * @brief 检查组件是否就绪
 * @return true 可用
 */
bool {{component_name}}_is_ready(void);

#ifdef __cplusplus
}
#endif
