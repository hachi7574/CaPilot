# BLE HID 组件剥离步骤（备选方案）

> 仅当必须用无线时才考虑 BLE HID。桌面外设首选 USB HID。

## 从立创实战派 14-handheld 例程剥离

源码位置：`A:\ESPIDF\Espressif5\frameworks\esp-idf-v5.5.4\examples\peripherals\lcd_touch\esp32_s3_lcd_touch\14-handheld\`

### 需要复制的文件

从 `main/bt/` 目录复制：
- `ble_hidd_demo.c/h`
- `esp_hidd_prf_api.c/h`
- `hid_dev.c/h`
- `hid_device_le_prf.c`
- `hidd_le_prf_int.h`

### 修改步骤

1. 从 `ble_hidd_demo.c` 删除无关 include：
   - `esp_wifi.h`（HID 不需要）
   - `esp_event.h`（除非用 event loop）
   - `esp_lvgl_port.h`（除非组件直接用 LVGL）

2. 把 `static uint16_t hid_conn_id` 和 `static bool sec_conn` 改为非 static，暴露给上层使用

3. 删除所有 LVGL 相关代码：
   - `btn1` / `btn2` event handler
   - `app_hid_ctrl` 函数

4. 组件 `CMakeLists.txt` 添加依赖：
   ```cmake
   REQUIRES "bt" "nvs_flash" "esp_driver_gpio"
   ```

### sdkconfig 必需项

```ini
CONFIG_BT_ENABLED=y
CONFIG_BT_BLE_42_FEATURES_SUPPORTED=y
```

## USB vs BLE 选型

| 维度 | USB HID (TinyUSB) | BLE HID (Bluedroid) |
|------|-------------------|---------------------|
| 配对 | 即插即用，零配对 | 需手动蓝牙配对 |
| 延迟 | <5ms | 10-50ms |
| 固件体积 | ~544KB | ~992KB（+45%） |
| DRAM 占用 | 248KB free | 157KB free（多耗 90KB） |
| 依赖 | espressif/esp_tinyusb ^2.0.0 | 内置 bt 组件 |
| 适用场景 | 桌面外设、有线连接 | 无线场景、移动设备 |

**推荐：** 桌面外设首选 USB HID。
