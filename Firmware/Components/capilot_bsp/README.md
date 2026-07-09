# capilot_bsp — 板级支持包

> 立创实战派 ESP32-S3 开发板的 BSP 组件，封装板载外设初始化与基本控制。

## 硬件覆盖

| 外设 | 接口 | 驱动方式 |
|------|------|----------|
| I2C 总线 (SDA=GPIO1, SCL=GPIO2) | I2C_NUM_0, 400kHz | ESP-IDF I2C Master (新 API) |
| PCA9557 IO 扩展器 | I2C 0x19 | 读-修改-写 IO 引脚 |
| LCD ST7789 (320x240) | SPI3_HOST, 80MHz | esp_lcd 框架 |
| 背光 PWM | GPIO42, LEDC 10-bit, 5kHz | 输出反相 (高电平=暗) |
| 触摸屏 FT6336 | I2C 0x38 | esp_lcd_touch_ft5x06 兼容驱动 |
| LVGL 集成 | — | lvgl + esp_lvgl_port |

## 初始化流程

### 快速启动（LVGL 全功能）

```c
capilot_bsp_i2c_init();         // 1. I2C 总线
capilot_bsp_pca9557_init();     // 2. PCA9557 IO 扩展
capilot_bsp_lvgl_start();       // 3. LCD + 触摸 + LVGL + 背光 = 一键搞定
```

### 裸屏模式（无 GUI）

```c
capilot_bsp_i2c_init();
capilot_bsp_pca9557_init();
capilot_bsp_lcd_init();
capilot_bsp_lcd_set_color(0x0000);  // 清屏
capilot_bsp_backlight_init();
capilot_bsp_backlight_on();
// capilot_bsp_lcd_draw_picture() / capilot_bsp_pa_enable() / ...
```

## Kconfig 配置

菜单路径：`CaPilot BSP Configuration`

| 开关 | 默认 | 说明 |
|------|------|------|
| `CAPILOT_BSP_TOUCH_ENABLE` | y | 使能触摸屏 (FT6336) |
| `CAPILOT_BSP_LVGL_ENABLE` | y | 使能 LVGL (需同时开启 TOUCH) |
| `CAPILOT_BSP_LCD_SPI_FREQ` | 80000000 | LCD SPI 时钟 (Hz) |
| `CAPILOT_BSP_LCD_BACKLIGHT_PWM_FREQ` | 5000 | 背光 PWM 频率 (Hz) |

## 依赖

| 组件 | 条件 |
|------|------|
| `freertos`, `esp_driver_gpio`, `esp_driver_i2c`, `esp_driver_spi`, `esp_driver_ledc`, `esp_lcd` | 始终 |
| `esp_lcd_touch`, `esp_lcd_touch_ft5x06` | `CONFIG_CAPILOT_BSP_TOUCH_ENABLE` |
| `lvgl`, `esp_lvgl_port` | `CONFIG_CAPILOT_BSP_LVGL_ENABLE` |

## I2C 总线复用

I2C 手柄通过 `capilot_bsp_i2c.h` 暴露给其他组件：

```c
#include "capilot_bsp_i2c.h"

// 获取总线句柄，创建自己的设备
capilot_bsp_i2c_bus_handle_t bus = capilot_bsp_i2c_get_bus_handle();
// 或直接用封装好的读写函数
capilot_bsp_i2c_read(dev_addr, reg, data, len);
capilot_bsp_i2c_write_byte(dev_addr, reg, val);
```

挂载在 I2C 总线上的设备：PCA9557 (0x19)、QMI8658 (0x6A)、FT6336 (0x38)、ES8311 (0x18)、ES7210 (0x41)。
