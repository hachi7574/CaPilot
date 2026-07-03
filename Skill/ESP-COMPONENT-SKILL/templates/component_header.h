#pragma once

#include <stdint.h>
#include <stdbool.h>        // 用 bool 必须包含
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 常量定义 */
#define {{COMPONENT_NAME_UPPER}}_CONSTANT  0x1234

/* API 函数 */
esp_err_t {{component_name}}_init(void);
bool {{component_name}}_is_ready(void);

#ifdef __cplusplus
}
#endif
