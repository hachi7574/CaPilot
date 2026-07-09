# 06 - LCD 图片显示

立创实战派 ESP32-S3 开发板外设示例 — 在 LCD 屏幕上显示图片。

## 功能

- 初始化 I2C、PCA9557 IO 扩展和 LCD 显示屏
- 在 320×240 的 LCD 上显示一张鹦鹉图片

## 学习要点

- `bsp_lcd_init()` LCD 初始化流程
- `lcd_draw_pictrue()` 绘制全屏图片
- 图片数据以 C 数组形式嵌入固件（`yingwu.h`）

## 硬件接线

- LCD 通过 SPI/并行接口连接（板载，无需额外接线）

## 编译与烧录

```bash
idf.py set-target esp32s3
idf.py build flash monitor
```

烧录后屏幕将显示鹦鹉图片。
