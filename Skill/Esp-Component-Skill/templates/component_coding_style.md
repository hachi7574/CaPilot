# ESP-IDF C 代码风格规范

本规范适用于 CaPilot 项目中所有 ESP-IDF 组件代码。目标：一致性、可读性、可维护性。

## 1. 命名规则

| 类别 | 风格 | 示例 |
|------|------|------|
| 函数（公开 API） | `snake_case`，前缀 `{comp}_` | `qmi8658_init()` `pca9557_write_pin()` |
| 函数（内部 static） | `snake_case`，无前缀或 `_` 前缀 | `_check_id()` `write_reg()` |
| 变量 | `snake_case` | `data_ready` `i2c_addr` |
| 全局/static 变量 | `snake_case`，加 `g_` / `s_` 前缀 | `s_imu_handle` `g_debug_mode` |
| 宏 / #define | `UPPER_SNAKE` | `QMI8658_I2C_ADDR` `MAX_RETRY_COUNT` |
| 枚举值 | `UPPER_SNAKE` | `MODE_IDLE` `STATE_RUNNING` |
| 类型名 | `snake_case` + `_t` 后缀 | `qmi8658_config_t` `sensor_data_t` |
| 结构体成员 | `snake_case` | `.sda_pin` `.odr_hz` |

## 2. 头文件规范

```c
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* === 公共常量 ======================================================== */

#define MY_COMP_I2C_ADDR       0x6B
#define MY_COMP_MAX_RETRY      3

/* === 类型定义 ======================================================== */

typedef enum {
    MY_COMP_MODE_LOW_POWER = 0,
    MY_COMP_MODE_NORMAL,
    MY_COMP_MODE_HIGH_PERF,
} my_comp_mode_t;

typedef struct {
    uint8_t     i2c_addr;
    uint16_t    odr_hz;
    my_comp_mode_t mode;
} my_comp_config_t;

/* === 默认配置宏（方便调用者用 = {0} + 部分覆盖） ===================== */

#define MY_COMP_DEFAULT_CONFIG() { \
    .i2c_addr = MY_COMP_I2C_ADDR,  \
    .odr_hz   = 100,               \
    .mode     = MY_COMP_MODE_NORMAL,\
}

/* === API 函数声明 ==================================================== */

/**
 * @brief 初始化组件
 * @param cfg 配置参数，传 NULL 使用默认值
 * @return ESP_OK 成功
 */
esp_err_t my_comp_init(const my_comp_config_t *cfg);

/**
 * @brief 反初始化，释放资源
 * @return ESP_OK 成功
 */
esp_err_t my_comp_deinit(void);

/**
 * @brief 检查组件是否就绪
 * @return true 可用
 */
bool my_comp_is_ready(void);

#ifdef __cplusplus
}
#endif
```

规范要点：
- **`#pragma once`** 替代 `#ifndef` 守卫
- **`extern "C"`** 包裹所有内容
- **分隔注释** `/* === xxx === */` 清晰分区
- **Doxygen 风格注释** `@brief` `@param` `@return` 放在声明前
- **默认配置宏** 用复合字面量 `{...}`，方便调用者 `= MY_DEFAULT_CONFIG()` 后覆盖个别字段
- **公开头文件只暴露需要的东西**，内部类型/enum 放 `.c` 里

## 3. 源文件规范

```c
/* === 头文件 ========================================================== */
#include "my_comp.h"
#include "esp_log.h"
#include "driver/i2c_master.h"

/* === 日志标签 ======================================================== */
static const char *TAG = "[my_comp]";

/* === 硬件常量（内部，不公开） ======================================== */
#define REG_WHO_AM_I    0x00
#define REG_CTRL1       0x10

/* === 模块级变量 ====================================================== */
static i2c_master_dev_handle_t s_dev_handle = NULL;
static bool s_initialized = false;

/* === 内部函数声明 ==================================================== */
static esp_err_t _read_reg(uint8_t reg, uint8_t *data, size_t len);
static esp_err_t _write_reg(uint8_t reg, uint8_t data);
static bool _check_chip_id(void);

/* === 公开 API 实现 =================================================== */

esp_err_t my_comp_init(const my_comp_config_t *cfg) {
    if (s_initialized) {
        ESP_LOGW(TAG, "already initialized, deinit first");
        my_comp_deinit();
    }
    // ... 实现
    s_initialized = true;
    ESP_LOGI(TAG, "init OK");
    return ESP_OK;
}

esp_err_t my_comp_deinit(void) {
    if (!s_initialized) return ESP_OK;
    // ... 释放资源
    s_initialized = false;
    return ESP_OK;
}

/* === 内部函数实现 ==================================================== */

/**
 * @brief  读寄存器（内部）
 * @note   QMI8658A datasheet §7.2, 支持多字节读取
 */
static esp_err_t _read_reg(uint8_t reg, uint8_t *data, size_t len) {
    // 注释注明手册出处！
    return i2c_master_transmit_receive(s_dev_handle, &reg, 1, data, len, 100);
}
```

规范要点：
- **头文件区**：自有头文件第一，标准库第二，ESP-IDF 库第三
- **TAG 格式**：`[comp_name]` 括号包裹
- **static 内部函数** 加 `_` 前缀以示区分
- **寄存器操作** 必须注释手册页码/章节号
- **magic number 用 #define**，不裸写

## 4. 错误处理

```c
// 公开 API：返回 esp_err_t，让调用者决定怎么处理
esp_err_t qmi8658_read_accel(float *x, float *y, float *z) {
    if (!s_initialized) return ESP_ERR_INVALID_STATE;
    if (!x || !y || !z) return ESP_ERR_INVALID_ARG;
    // ...
}

// 内部初始化：致命错误用 ESP_ERROR_CHECK（直接 abort）
ESP_ERROR_CHECK(i2c_master_probe(handle, &addr, 100));

// 内部运行时：记录日志 + 返回错误码
if (!_check_chip_id()) {
    ESP_LOGE(TAG, "chip ID mismatch");
    return ESP_ERR_NOT_FOUND;
}

// I2C 通信：重试 + 超时
for (int retry = 0; retry < MAX_RETRY; retry++) {
    err = i2c_master_transmit(...);
    if (err == ESP_OK) break;
    vTaskDelay(pdMS_TO_TICKS(10));
}
if (err != ESP_OK) {
    ESP_LOGE(TAG, "I2C write timeout after %d retries", MAX_RETRY);
    return ESP_ERR_TIMEOUT;
}
```

## 5. 日志规范

| 级别 | 使用场景 | 示例 |
|------|---------|------|
| `ESP_LOGE` | 通信失败、硬件异常、配置错误 | `"I2C timeout"` `"chip ID mismatch"` |
| `ESP_LOGW` | 可恢复的异常、降级运行 | `"retry %d/3"` `"using default config"` |
| `ESP_LOGI` | 初始化完成、模式切换 | `"init OK"` `"mode: normal"` |
| `ESP_LOGD` | 调试信息（默认不输出） | `"reg[0x%02x] = 0x%02x"` |

## 6. 其他规则

- **文件编码**：UTF-8 without BOM
- **缩进**：4 空格，不用 Tab
- **行宽**：建议 ≤ 100 字符
- **大括号**：K&R 风格（函数左括号另起一行，if/for/while 左括号同行）
- **注释**：英文或中文均可，但同一文件保持一致
- **空行**：逻辑块之间空一行
- **类型**：优先用 `stdint.h` 类型（`uint8_t` `int32_t`），不用 `int` `char` 做存储

## 7. 禁止事项

- 不用 `goto`（错误处理用 `esp_err_t` 返回值链）
- 不在头文件里定义变量（只声明）
- 不在 ISR 里 `ESP_LOGI`（用 `ESP_EARLY_LOGI` 或放 task 里打）
- 不用 `delay()` / `sleep()` 做同步（用信号量/事件组）
- 不用 `malloc`/`free` 动态分配（优先静态分配，若必须则 init 时分配、deinit 时释放）
