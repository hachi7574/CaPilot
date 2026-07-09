# 01 - BOOT 按键中断检测

立创实战派 ESP32-S3 开发板入门示例 — GPIO 中断检测。

## 功能

- 配置 GPIO 0（板载 BOOT 按键）为下降沿中断
- 按下按键时通过串口打印 GPIO 状态
- 演示 ESP-IDF 的 GPIO 中断配置与队列传参

## 学习要点

- `gpio_config()` 配置 GPIO 模式、上下拉与中断类型
- `gpio_install_isr_service()` 安装中断服务
- `gpio_isr_handler_add()` 注册中断回调
- `xQueueCreate` / `xQueueSendFromISR` 在中断中传递数据
- `xQueueReceive` 在任务中处理中断事件

## 硬件接线

- BOOT 按键：GPIO 0（板载，无需额外接线）

## 编译与烧录

```bash
idf.py set-target esp32s3
idf.py build flash monitor
```

按下 BOOT 键，终端将打印 `GPIO[0] intr, val: 0`。
