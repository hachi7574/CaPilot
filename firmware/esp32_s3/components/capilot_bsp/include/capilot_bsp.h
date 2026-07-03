/**
 * @file capilot_bsp.h
 * @brief CaPilot Board Support Package — 立创实战派 ESP32-S3
 *
 * 板级硬件抽象层，封装开发板所有板载外设的初始化和控制。
 * 核心功能（I2C/PCA9557）始终编译；触摸/LVGL 通过 Kconfig 按需启用。
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ================================================================
 *  I2C 总线
 * ================================================================
 *  SDA: GPIO 1, SCL: GPIO 2, 速率: 100kHz
 *  挂载设备: PCA9557(0x19), QMI8658(0x6A), FT6336(0x38),
 *            ES8311(0x18), ES7210(0x41)
 * ================================================================ */

#define CAPILOT_BSP_I2C_SDA         (GPIO_NUM_1)
#define CAPILOT_BSP_I2C_SCL         (GPIO_NUM_2)
#define CAPILOT_BSP_I2C_NUM         (0)
#define CAPILOT_BSP_I2C_FREQ_HZ     100000

/**
 * @brief 初始化 I2C 主机总线
 * @return ESP_OK 成功，否则失败
 */
esp_err_t capilot_bsp_i2c_init(void);

/* ================================================================
 *  PCA9557 IO 扩展器 (I2C 地址 0x19)
 * ================================================================
 *  IO0: LCD_CS       — LCD 片选（低电平有效）
 *  IO1: PA_EN        — 音频功放使能（高电平=开启）
 *  IO2: DVP_PWDN     — 摄像头电源（低电平=开启）
 * ================================================================ */

#define CAPILOT_BSP_PCA9557_ADDR    0x19
#define CAPILOT_BSP_LCD_CS          BIT(0)
#define CAPILOT_BSP_PA_EN           BIT(1)
#define CAPILOT_BSP_DVP_PWDN        BIT(2)

/**
 * @brief 初始化 PCA9557 IO 扩展器
 * @note 必须在 I2C 初始化之后调用
 */
void capilot_bsp_pca9557_init(void);

/**
 * @brief 控制 LCD 片选
 * @param level 0=选中, 1=取消选中
 */
void capilot_bsp_lcd_cs(uint8_t level);

/**
 * @brief 控制音频功放使能
 * @param enable true=开启功放, false=关闭
 */
void capilot_bsp_pa_enable(bool enable);

/**
 * @brief 控制摄像头电源
 * @param enable true=开启摄像头, false=关闭
 */
void capilot_bsp_camera_power(bool enable);

/* ================================================================
 *  LCD 显示屏 — ST7789（SPI3_HOST, 320×240, 16-bit RGB565）
 * ================================================================
 *  始终可用。不依赖 LVGL。
 * ================================================================ */

#define CAPILOT_BSP_LCD_H_RES       320
#define CAPILOT_BSP_LCD_V_RES       240

#if CONFIG_CAPILOT_BSP_LVGL_ENABLE
#include "esp_lcd_types.h"
#endif

/**
 * @brief 初始化 LCD 显示屏（SPI + ST7789 驱动）
 * @return ESP_OK 成功，否则失败
 */
esp_err_t capilot_bsp_lcd_init(void);

/**
 * @brief 在 LCD 上绘制全尺寸图片
 * @param x_start, y_start 起始坐标
 * @param x_end, y_end 结束坐标
 * @param image RGB565 像素数据
 */
void capilot_bsp_lcd_draw_picture(
    int x_start, int y_start,
    int x_end, int y_end,
    const uint8_t *image);

/**
 * @brief 设置全屏为单一颜色
 * @param color RGB565 颜色值
 */
void capilot_bsp_lcd_set_color(uint16_t color);

#if CONFIG_CAPILOT_BSP_LVGL_ENABLE
/**
 * @brief 获取 LCD panel 句柄（给 LVGL 使用）
 */
esp_lcd_panel_handle_t capilot_bsp_lcd_get_panel_handle(void);

/**
 * @brief 获取 LCD panel IO 句柄（给 LVGL 使用）
 */
esp_lcd_panel_io_handle_t capilot_bsp_lcd_get_io_handle(void);
#endif

/* ================================================================
 *  LCD 背光（PWM, GPIO 42, 5kHz, 10-bit）
 * ================================================================ */

#define CAPILOT_BSP_LCD_BACKLIGHT   (GPIO_NUM_42)

/**
 * @brief 初始化背光 PWM
 * @return ESP_OK 成功，否则失败
 */
esp_err_t capilot_bsp_backlight_init(void);

/**
 * @brief 设置背光亮度
 * @param percent 亮度百分比 (0~100)
 * @return ESP_OK 成功，否则失败
 */
esp_err_t capilot_bsp_backlight_set(int percent);

/** @brief 关闭背光（快捷方式） */
void capilot_bsp_backlight_off(void);

/** @brief 开启背光（快速设为 100%） */
void capilot_bsp_backlight_on(void);

/* ================================================================
 *  触摸屏 — FT6336（I2C, 需 Kconfig: CAPILOT_BSP_TOUCH_ENABLE）
 * ================================================================ */

#if CONFIG_CAPILOT_BSP_TOUCH_ENABLE
#include "esp_lcd_touch.h"
#include "esp_lcd_touch_ft5x06.h"

/**
 * @brief 初始化触摸屏
 * @param[out] ret_touch 返回触摸屏句柄
 * @return ESP_OK 成功，否则失败
 */
esp_err_t capilot_bsp_touch_init(esp_lcd_touch_handle_t *ret_touch);
#endif

/* ================================================================
 *  LVGL 集成（需 Kconfig: CAPILOT_BSP_LVGL_ENABLE）
 *  一键初始化 LCD + 触摸 + LVGL + 背光
 * ================================================================ */

#if CONFIG_CAPILOT_BSP_LVGL_ENABLE
#include "lvgl.h"
#include "esp_lvgl_port.h"

/**
 * @brief 一键启动 LVGL（LCD + 触摸 + 背光）
 * @note 这是最常用的入口函数，内部依次调用：
 *       lcd_init -> touch_init -> lvgl_port_init -> backlight_on
 */
void capilot_bsp_lvgl_start(void);

/**
 * @brief 获取 LVGL 显示句柄
 */
lv_disp_t *capilot_bsp_lvgl_get_display(void);

/**
 * @brief 获取 LVGL 输入设备句柄（触摸）
 */
lv_indev_t *capilot_bsp_lvgl_get_indev(void);
#endif

#ifdef __cplusplus
}
#endif
