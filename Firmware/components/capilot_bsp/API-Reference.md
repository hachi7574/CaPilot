# capilot_bsp API 参考

## I2C 总线 (`capilot_bsp_i2c.h`)

### 引脚宏

```c
#define CAPILOT_BSP_I2C_SDA         GPIO_NUM_1
#define CAPILOT_BSP_I2C_SCL         GPIO_NUM_2
#define CAPILOT_BSP_I2C_NUM         0
#define CAPILOT_BSP_I2C_FREQ_HZ     400000
```

### 类型

```c
typedef void *capilot_bsp_i2c_bus_handle_t;  // 不透明句柄，实际为 i2c_master_bus_handle_t
```

### 函数

---

#### `capilot_bsp_i2c_init()`

```c
esp_err_t capilot_bsp_i2c_init(void);
```

初始化 I2C 主机总线（SDA=GPIO1, SCL=GPIO2, 400kHz）。可重入——重复调用返回 `ESP_OK` 并打 WARN 日志。

| 返回值 | 含义 |
|--------|------|
| `ESP_OK` | 成功 |
| 其他 | ESP-IDF 标准错误码 |

---

#### `capilot_bsp_i2c_get_bus_handle()`

```c
capilot_bsp_i2c_bus_handle_t capilot_bsp_i2c_get_bus_handle(void);
```

获取 I2C 总线句柄，供其他组件创建设备。未初始化时返回 NULL。

---

#### `capilot_bsp_i2c_read()`

```c
esp_err_t capilot_bsp_i2c_read(uint8_t dev_addr, uint8_t reg, uint8_t *data, size_t len);
```

向 I2C 设备发送 1 字节寄存器地址，然后读取 len 字节。

| 参数 | 说明 |
|------|------|
| `dev_addr` | 7-bit I2C 设备地址 |
| `reg` | 目标寄存器地址 |
| `data` | 输出缓冲区 |
| `len` | 读取字节数 |

| 返回值 | 含义 |
|--------|------|
| `ESP_OK` | 成功 |
| `ESP_ERR_INVALID_STATE` | 总线未初始化 |
| 其他 | I2C 传输错误 |

> 内部缓存最多 4 个 I2C 设备句柄，避免重复创建/销毁。

---

#### `capilot_bsp_i2c_write_byte()`

```c
esp_err_t capilot_bsp_i2c_write_byte(uint8_t dev_addr, uint8_t reg, uint8_t val);
```

向 I2C 设备指定寄存器写入 1 字节。

| 参数 | 说明 |
|------|------|
| `dev_addr` | 7-bit I2C 设备地址 |
| `reg` | 目标寄存器地址 |
| `val` | 要写入的字节 |

返回值同上。

---

## PCA9557 IO 扩展器

### 宏

```c
#define CAPILOT_BSP_PCA9557_ADDR    0x19
#define CAPILOT_BSP_LCD_CS          BIT(0)   // IO0 — LCD SPI 片选 (低电平有效)
#define CAPILOT_BSP_PA_EN           BIT(1)   // IO1 — 音频功放使能 (高=开)
#define CAPILOT_BSP_DVP_PWDN        BIT(2)   // IO2 — 摄像头电源 (低=开)
```

### 函数

---

#### `capilot_bsp_pca9557_init()`

```c
void capilot_bsp_pca9557_init(void);
```

初始化 PCA9557：IO0/1/2 设为输出，初始状态 CS=1（取消选中）、PA_EN=0（关闭）、DVP_PWDN=1（关闭）。

前置条件：`capilot_bsp_i2c_init()`。

---

#### `capilot_bsp_lcd_cs()`

```c
void capilot_bsp_lcd_cs(uint8_t level);
```

控制 LCD 片选。`level=0` 选中，`level=1` 取消选中。

---

#### `capilot_bsp_pa_enable()`

```c
void capilot_bsp_pa_enable(bool enable);
```

控制音频功放。`enable=true` 开启，`false` 关闭。

---

#### `capilot_bsp_camera_power()`

```c
void capilot_bsp_camera_power(bool enable);
```

控制摄像头电源。`enable=true` 开启（DVP_PWDN 拉低），`false` 关闭。

---

## LCD 显示

### 宏

```c
#define CAPILOT_BSP_LCD_H_RES       320
#define CAPILOT_BSP_LCD_V_RES       240
```

### 函数

---

#### `capilot_bsp_lcd_init()`

```c
esp_err_t capilot_bsp_lcd_init(void);
```

初始化 ST7789 LCD（SPI3_HOST, 80MHz, 320×240, RGB565）。内部完成：SPI 总线 → panel IO → ST7789 驱动 → 复位 → 初始化序列 → 颜色反转 → XY 交换 → 镜像 → 开显示。

前置条件：`capilot_bsp_i2c_init()` + `capilot_bsp_pca9557_init()`。

---

#### `capilot_bsp_lcd_draw_picture()`

```c
void capilot_bsp_lcd_draw_picture(
    int x_start, int y_start,
    int x_end, int y_end,
    const uint8_t *image);
```

在 LCD 上绘制矩形区域图片。

| 参数 | 说明 |
|------|------|
| `x_start, y_start` | 矩形左上角坐标 |
| `x_end, y_end` | 矩形右下角坐标 |
| `image` | RGB565 格式像素数据 |

内部先分配内存（优先 SPIRAM），调用 `esp_lcd_panel_draw_bitmap()` 后释放。

---

#### `capilot_bsp_lcd_set_color()`

```c
void capilot_bsp_lcd_set_color(uint16_t color);
```

将整个屏幕填充为指定颜色。

| 参数 | 说明 |
|------|------|
| `color` | RGB565 颜色值（如 `0x0000`=黑, `0xFFFF`=白） |

逐行填充，每次分配一行缓冲区。

---

#### `capilot_bsp_lcd_get_panel_handle()` （LVGL 启用时）

```c
esp_lcd_panel_handle_t capilot_bsp_lcd_get_panel_handle(void);
```

获取 LCD panel 句柄，供 LVGL 使用。

---

#### `capilot_bsp_lcd_get_io_handle()` （LVGL 启用时）

```c
esp_lcd_panel_io_handle_t capilot_bsp_lcd_get_io_handle(void);
```

获取 LCD panel IO 句柄，供 LVGL 使用。

---

## 背光控制

#### `capilot_bsp_backlight_init()`

```c
esp_err_t capilot_bsp_backlight_init(void);
```

初始化背光 PWM（GPIO42, LEDC, 10-bit）。频率由 Kconfig `CAPILOT_BSP_LCD_BACKLIGHT_PWM_FREQ` 配置，默认 5kHz。

---

#### `capilot_bsp_backlight_set()`

```c
esp_err_t capilot_bsp_backlight_set(int percent);
```

设置背光亮度百分比。

| 参数 | 说明 |
|------|------|
| `percent` | 0~100，超范围自动钳位 |

duty = 1023 × percent / 100。

---

#### `capilot_bsp_backlight_off()`

```c
void capilot_bsp_backlight_off(void);
```

关闭背光（等效 `backlight_set(0)`）。

---

#### `capilot_bsp_backlight_on()`

```c
void capilot_bsp_backlight_on(void);
```

开启背光至最大（等效 `backlight_set(100)`）。

---

## 触摸屏（条件编译：`CONFIG_CAPILOT_BSP_TOUCH_ENABLE`）

#### `capilot_bsp_touch_init()`

```c
esp_err_t capilot_bsp_touch_init(esp_lcd_touch_handle_t *ret_touch);
```

初始化 FT6336 电容触摸屏（I2C 地址 0x38）。

| 参数 | 说明 |
|------|------|
| `ret_touch` | [输出] 触摸屏句柄 |

坐标映射：`swap_xy=1`, `mirror_x=1`, `mirror_y=0`（与 LCD 方向对齐）。

---

## LVGL 集成（条件编译：`CONFIG_CAPILOT_BSP_LVGL_ENABLE`）

#### `capilot_bsp_lvgl_start()`

```c
void capilot_bsp_lvgl_start(void);
```

一键启动 LVGL。内部完整序列：

1. LVGL 端口初始化
2. LCD 初始化 + 清屏（黑色）
3. 创建 LVGL display（320×240, 双缓冲, DMA）
4. 触摸初始化 + 绑定 LVGL input device
5. 背光初始化 + 开启

内部使用 `assert` 做错误检查。

---

#### `capilot_bsp_lvgl_get_display()`

```c
lv_disp_t *capilot_bsp_lvgl_get_display(void);
```

获取 LVGL 显示句柄，未初始化时返回 NULL。

---

#### `capilot_bsp_lvgl_get_indev()`

```c
lv_indev_t *capilot_bsp_lvgl_get_indev(void);
```

获取 LVGL 输入设备句柄（触摸），未初始化时返回 NULL。
