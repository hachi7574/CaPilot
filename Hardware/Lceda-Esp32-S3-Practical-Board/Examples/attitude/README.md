# 02 - QMI8658 姿态传感器

立创实战派 ESP32-S3 开发板外设示例 — 读取 QMI8658 六轴姿态传感器。

## 功能

- 通过 I2C 初始化 QMI8658 加速度计/陀螺仪
- 循环读取 X/Y/Z 轴倾角（从加速度数据计算）
- 串口打印实时角度数据

## 学习要点

- I2C 总线初始化与设备寻址
- QMI8658 寄存器读写时序
- 从加速度计原始数据计算 Pitch/Roll 角度
- `atan2f()` 三角函数在嵌入式中的应用

## 硬件接线

- QMI8658 通过 I2C 连接（SDA: GPIO 1, SCL: GPIO 2）
- 地址：0x6A

## 编译与烧录

```bash
idf.py set-target esp32s3
idf.py build flash monitor
```

终端将每秒输出 `angle_x = xx.x  angle_y = xx.x  angle_z = xx.x`。
