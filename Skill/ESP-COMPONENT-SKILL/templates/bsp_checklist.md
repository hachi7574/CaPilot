# BSP 精简驱动裁剪要点

从立创实战派 handheld 例程的 `esp32_s3_szp.c` 裁剪 BSP 时：

## 保留

- I2C 总线初始化
- PCA9557 IO 扩展初始化
- LCD 屏初始化（esp_lcd_panel_io_spi + esp_lcd_new_panel_st7789）
- 触摸屏初始化（esp_lcd_touch_ft5x06）
- LVGL 初始化（esp_lvgl_port）

## 删除（按应用需求）

- 摄像头驱动
- 音频驱动（ES7210 麦克风、ES8311 编解码）
- SD 卡驱动
- SPIFFS 文件系统（如果应用不需要存储）

## 注意事项

1. **删除 `freertos/eventgroups.h`** — 如果不用 event group，这个 include 在 v5.5.4 会报错（路径变为 `freertos/event_groups.h`）

2. **`ESP_LCD_TOUCH_IO_I2C_FT5x06_CONFIG()` 宏** — 不要设置 `scl_speed_hz`，v5.5.4 legacy I2C 驱动不允许，速度由 `i2c_config_t.master.clk_speed` 决定

3. **PCA9557 中断引脚** — 如果不用中断，可以只做初始化不注册 ISR

4. **LCD 背光** — PCA9557 的某个输出引脚控制背光，初始化时记得拉高，否则屏幕黑屏

## 参考

`{{PROJECT_ROOT}}/Firmware/esp32_s3/keyboard_demo/main/bsp.c` 是已验证的精简版本。
