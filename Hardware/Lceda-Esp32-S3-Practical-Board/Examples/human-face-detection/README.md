# 13 - AI 人脸检测

立创实战派 ESP32-S3 开发板 AI 示例 — 基于 ESP-DL 的实时人脸检测。

## 功能

- 先显示鹦鹉图片作为启动画面
- 初始化摄像头传感器
- 运行 ESP-DL 深度学习推理引擎
- 实时检测摄像头画面中的人脸，并用方框标记
- 将检测结果叠加显示到 LCD 屏幕

## 学习要点

- ESP-DL（Espressif Deep Learning）框架
- 基于 ESP32-S3 的 AI 推理（集成向量扩展指令集）
- 人脸检测神经网络模型部署
- 摄像头采集 + AI 推理 + LCD 显示流水线
- C++ 与 C 混合编程（`extern "C"`）

## 硬件接线

- 摄像头通过 DVP 接口连接（板载）
- 使用 ESP-DL 组件（位于 `components/esp-dl/`）

## 编译与烧录

```bash
idf.py set-target esp32s3
idf.py build flash monitor
```

烧录后摄像头对准人脸，屏幕将实时显示检测框。
