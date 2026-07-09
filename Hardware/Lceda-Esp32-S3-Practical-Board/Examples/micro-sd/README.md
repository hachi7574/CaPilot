# 03 - MicroSD 卡读写

立创实战派 ESP32-S3 开发板外设示例 — MicroSD 卡文件读写。

## 功能

- 使用 SDMMC 外设驱动 MicroSD 卡（1 线模式）
- 自动挂载 FAT 文件系统
- 创建文本文件并写入内容
- 读取文件内容并通过串口打印
- 完成后安全卸载文件系统

## 学习要点

- `sdmmc_host_t` / `sdmmc_slot_config_t` 配置 SDMMC 接口
- `esp_vfs_fat_sdmmc_mount()` 挂载 FAT 文件系统
- `fopen` / `fprintf` / `fgets` 标准 C 文件操作
- `esp_vfs_fat_sdcard_unmount()` 安全卸载

## 硬件接线

- SD_CLK: GPIO 47
- SD_CMD: GPIO 48
- SD_D0:  GPIO 21
- 1 线 SD 模式（宽度 = 1）

## 编译与烧录

```bash
idf.py set-target esp32s3
idf.py build flash monitor
```

插入 MicroSD 卡，程序将自动挂载并在卡上创建 `你好hello.txt` 文件。
