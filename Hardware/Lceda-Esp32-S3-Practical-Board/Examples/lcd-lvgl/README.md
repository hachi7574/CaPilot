# 08 - LVGL 图形框架演示

立创实战派 ESP32-S3 开发板外设示例 — LVGL 图形库性能与控件演示。

## 功能

- 使用 `bsp_lvgl_start()` 初始化 LVGL + LCD + 触摸屏
- 默认运行 `lv_demo_benchmark()` 性能基准测试
- 可切换为以下 LVGL 内置 Demo：
  - `lv_demo_keypad_encoder()` — 键盘编码器
  - `lv_demo_music()` — 音乐播放器界面
  - `lv_demo_stress()` — 压力测试
  - `lv_demo_widgets()` — 控件展示

## 学习要点

- LVGL 在 ESP32-S3 上的移植与初始化
- 触摸屏与 LVGL 输入设备的绑定
- LVGL 内置 Demos 的切换使用
- `lv_demo_benchmark()` 评估 MCU 图形性能

## 编译与烧录

```bash
idf.py set-target esp32s3
idf.py build flash monitor
```

通过修改 `main.c` 中注释的 Demo 行来切换不同演示。
