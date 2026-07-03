/**
 * @file capilot_bsp.c
 * @brief CaPilot BSP — 立创实战派 ESP32-S3 板级支持包实现
 *
 * 基于已验证的 keyboard_demo/bsp.c 和 handheld/esp32_s3_szp.c 整合。
 * 核心模块（I2C/PCA9557/LCD/Backlight）始终编译；
 * 触摸和 LVGL 通过 Kconfig 按需启用。
 */
#include "capilot_bsp.h"
#include <string.h>
#include "esp_log.h"
#include "esp_check.h"
#include "driver/i2c.h"
#include "driver/spi_master.h"
#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#if CONFIG_CAPILOT_BSP_LVGL_ENABLE
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#endif

static const char *TAG = "capilot_bsp";

/* I2C port number, stored after i2c_init for use by PCA9557 */
static int s_i2c_port = -1;

/******************************************************************************/
/*                            I2C 总线                                         */
/******************************************************************************/

esp_err_t capilot_bsp_i2c_init(void)
{
    ESP_LOGI(TAG, "I2C init: SDA=GPIO%d, SCL=GPIO%d, freq=%dHz",
             CAPILOT_BSP_I2C_SDA, CAPILOT_BSP_I2C_SCL, CAPILOT_BSP_I2C_FREQ_HZ);

    const i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = CAPILOT_BSP_I2C_SDA,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = CAPILOT_BSP_I2C_SCL,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = CAPILOT_BSP_I2C_FREQ_HZ,
    };

    ESP_RETURN_ON_ERROR(
        i2c_param_config(CAPILOT_BSP_I2C_NUM, &conf),
        TAG, "I2C param config failed");

    ESP_RETURN_ON_ERROR(
        i2c_driver_install(CAPILOT_BSP_I2C_NUM, conf.mode, 0, 0, 0),
        TAG, "I2C driver install failed");

    s_i2c_port = CAPILOT_BSP_I2C_NUM;
    ESP_LOGI(TAG, "I2C initialized successfully");
    return ESP_OK;
}

/******************************************************************************/
/*                         PCA9557 IO 扩展器                                   */
/******************************************************************************/

static esp_err_t pca9557_read_reg(uint8_t reg, uint8_t *data, size_t len)
{
    return i2c_master_write_read_device(
        s_i2c_port, CAPILOT_BSP_PCA9557_ADDR,
        &reg, 1, data, len, 1000 / portTICK_PERIOD_MS);
}

static esp_err_t pca9557_write_reg(uint8_t reg, uint8_t data)
{
    uint8_t buf[2] = {reg, data};
    return i2c_master_write_to_device(
        s_i2c_port, CAPILOT_BSP_PCA9557_ADDR,
        buf, sizeof(buf), 1000 / portTICK_PERIOD_MS);
}

static esp_err_t pca9557_set_pin(uint8_t pin_mask, uint8_t level)
{
    uint8_t val = 0;
    esp_err_t ret = pca9557_read_reg(0x01, &val, 1);  // OUTPUT_PORT
    if (ret != ESP_OK) return ret;

    if (level)  val |= pin_mask;
    else        val &= ~pin_mask;

    return pca9557_write_reg(0x01, val);
}

void capilot_bsp_pca9557_init(void)
{
    ESP_LOGI(TAG, "PCA9557 init (addr=0x%02X)", CAPILOT_BSP_PCA9557_ADDR);

    /* Set initial output: LCD_CS=1, PA_EN=0, DVP_PWDN=1 */
    pca9557_write_reg(0x01, 0x05);  // OUTPUT_PORT

    /* Set IO0/1/2 as output, rest as input */
    pca9557_write_reg(0x03, 0xf8);  // CONFIGURATION_PORT

    ESP_LOGI(TAG, "PCA9557 initialized");
}

void capilot_bsp_lcd_cs(uint8_t level)
{
    pca9557_set_pin(CAPILOT_BSP_LCD_CS, level);
}

void capilot_bsp_pa_enable(bool enable)
{
    pca9557_set_pin(CAPILOT_BSP_PA_EN, enable ? 1 : 0);
}

void capilot_bsp_camera_power(bool enable)
{
    /* DVP_PWDN: low = camera on, high = camera off */
    pca9557_set_pin(CAPILOT_BSP_DVP_PWDN, enable ? 0 : 1);
}

/******************************************************************************/
/*                            LCD 背光（PWM）                                  */
/******************************************************************************/

#define BACKLIGHT_LEDC_CH       LEDC_CHANNEL_0
#define BACKLIGHT_LEDC_TIMER    0
#define BACKLIGHT_DUTY_RES      10   /* 10-bit = 0~1023 */
#define BACKLIGHT_FREQ_HZ       CONFIG_CAPILOT_BSP_LCD_BACKLIGHT_PWM_FREQ

esp_err_t capilot_bsp_backlight_init(void)
{
    const ledc_channel_config_t ch = {
        .gpio_num       = CAPILOT_BSP_LCD_BACKLIGHT,
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = BACKLIGHT_LEDC_CH,
        .intr_type      = LEDC_INTR_DISABLE,
        .timer_sel      = BACKLIGHT_LEDC_TIMER,
        .duty           = 0,
        .hpoint         = 0,
        .flags.output_invert = true,
    };
    const ledc_timer_config_t timer = {
        .speed_mode      = LEDC_LOW_SPEED_MODE,
        .duty_resolution = BACKLIGHT_DUTY_RES,
        .timer_num       = BACKLIGHT_LEDC_TIMER,
        .freq_hz         = BACKLIGHT_FREQ_HZ,
        .clk_cfg         = LEDC_AUTO_CLK,
    };

    ESP_RETURN_ON_ERROR(ledc_timer_config(&timer), TAG, "LEDC timer config failed");
    ESP_RETURN_ON_ERROR(ledc_channel_config(&ch),  TAG, "LEDC channel config failed");

    ESP_LOGI(TAG, "Backlight PWM initialized (%dHz, %d-bit)", BACKLIGHT_FREQ_HZ, BACKLIGHT_DUTY_RES);
    return ESP_OK;
}

esp_err_t capilot_bsp_backlight_set(int percent)
{
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;

    uint32_t duty = (1023 * percent) / 100;
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, BACKLIGHT_LEDC_CH, duty));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, BACKLIGHT_LEDC_CH));
    return ESP_OK;
}

void capilot_bsp_backlight_off(void)
{
    capilot_bsp_backlight_set(0);
}

void capilot_bsp_backlight_on(void)
{
    capilot_bsp_backlight_set(100);
}

/******************************************************************************/
/*                         LCD 显示屏（ST7789）                                */
/******************************************************************************/

#if CONFIG_CAPILOT_BSP_LVGL_ENABLE
static esp_lcd_panel_handle_t s_panel_handle = NULL;
static esp_lcd_panel_io_handle_t s_io_handle = NULL;
#else
static void *s_panel_handle = NULL;
static void *s_io_handle = NULL;
#endif

#define LCD_SPI_CLK_FREQ    CONFIG_CAPILOT_BSP_LCD_SPI_FREQ
#define LCD_SPI_HOST        SPI3_HOST
#define LCD_CMD_BITS        8
#define LCD_PARAM_BITS      8
#define LCD_BITS_PER_PIXEL  16
#define LCD_H_RES           320
#define LCD_V_RES           240
#define LCD_PIN_MOSI        (GPIO_NUM_40)
#define LCD_PIN_CLK         (GPIO_NUM_41)
#define LCD_PIN_DC          (GPIO_NUM_39)
#define LCD_PIN_RST         (GPIO_NUM_NC)
#define LCD_PIN_CS          (GPIO_NUM_NC)  /* CS via PCA9557 */

esp_err_t capilot_bsp_lcd_init(void)
{
    ESP_LOGI(TAG, "LCD init: ST7789 320x240 SPI");

    /* ---- SPI bus ---- */
    const spi_bus_config_t buscfg = {
        .sclk_io_num   = LCD_PIN_CLK,
        .mosi_io_num   = LCD_PIN_MOSI,
        .miso_io_num   = GPIO_NUM_NC,
        .quadwp_io_num = GPIO_NUM_NC,
        .quadhd_io_num = GPIO_NUM_NC,
        .max_transfer_sz = LCD_H_RES * 40 * sizeof(uint16_t),
    };
    ESP_RETURN_ON_ERROR(
        spi_bus_initialize(LCD_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO),
        TAG, "SPI bus init failed");

    /* ---- Panel IO (SPI) ---- */
    const esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num   = LCD_PIN_DC,
        .cs_gpio_num   = LCD_PIN_CS,
        .pclk_hz       = LCD_SPI_CLK_FREQ,
        .lcd_cmd_bits  = LCD_CMD_BITS,
        .lcd_param_bits = LCD_PARAM_BITS,
        .spi_mode      = 2,
        .trans_queue_depth = 10,
    };
    ESP_RETURN_ON_ERROR(
        esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_SPI_HOST,
                                 &io_config, &s_io_handle),
        TAG, "Panel IO init failed");

    /* ---- LCD controller (ST7789) ---- */
    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num  = LCD_PIN_RST,
        .rgb_ele_order   = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel  = LCD_BITS_PER_PIXEL,
    };
    ESP_RETURN_ON_ERROR(
        esp_lcd_new_panel_st7789(s_io_handle, &panel_config, &s_panel_handle),
        TAG, "Panel driver init failed");

    /* ---- Panel init sequence ---- */
    esp_lcd_panel_reset(s_panel_handle);
    capilot_bsp_lcd_cs(0);
    esp_lcd_panel_init(s_panel_handle);
    esp_lcd_panel_invert_color(s_panel_handle, true);
    esp_lcd_panel_swap_xy(s_panel_handle, true);
    esp_lcd_panel_mirror(s_panel_handle, true, false);
    esp_lcd_panel_disp_on_off(s_panel_handle, true);

    ESP_LOGI(TAG, "LCD initialized (SPI %dHz)", LCD_SPI_CLK_FREQ);
    return ESP_OK;
}

void capilot_bsp_lcd_draw_picture(
    int x_start, int y_start,
    int x_end, int y_end,
    const uint8_t *image)
{
    size_t pixels_size = (x_end - x_start) * (y_end - y_start) * 2;
    uint16_t *pixels = heap_caps_malloc(pixels_size,
                                         MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
    if (!pixels) {
        ESP_LOGE(TAG, "Not enough memory for LCD draw (%zu bytes)", pixels_size);
        return;
    }
    memcpy(pixels, image, pixels_size);
    esp_lcd_panel_draw_bitmap(s_panel_handle,
                               x_start, y_start, x_end, y_end, pixels);
    heap_caps_free(pixels);
}

void capilot_bsp_lcd_set_color(uint16_t color)
{
    uint16_t *buf = heap_caps_malloc(LCD_H_RES * sizeof(uint16_t),
                                      MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
    if (!buf) {
        ESP_LOGE(TAG, "Not enough memory for LCD clear");
        return;
    }
    for (size_t i = 0; i < LCD_H_RES; i++) {
        buf[i] = color;
    }
    for (int y = 0; y < LCD_V_RES; y++) {
        esp_lcd_panel_draw_bitmap(s_panel_handle, 0, y, LCD_H_RES, y + 1, buf);
    }
    free(buf);
}

#if CONFIG_CAPILOT_BSP_LVGL_ENABLE
esp_lcd_panel_handle_t capilot_bsp_lcd_get_panel_handle(void)
{
    return s_panel_handle;
}

esp_lcd_panel_io_handle_t capilot_bsp_lcd_get_io_handle(void)
{
    return s_io_handle;
}
#endif

/******************************************************************************/
/*                   触摸屏 FT6336（条件编译）                                 */
/******************************************************************************/

#if CONFIG_CAPILOT_BSP_TOUCH_ENABLE

esp_err_t capilot_bsp_touch_init(esp_lcd_touch_handle_t *ret_touch)
{
    ESP_LOGI(TAG, "Touch init: FT6336 via I2C");

    esp_lcd_touch_config_t tp_cfg = {
        .x_max = CAPILOT_BSP_LCD_V_RES,
        .y_max = CAPILOT_BSP_LCD_H_RES,
        .rst_gpio_num = GPIO_NUM_NC,
        .int_gpio_num = GPIO_NUM_NC,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = 1,
            .mirror_x = 1,
            .mirror_y = 0,
        },
    };

    esp_lcd_panel_io_handle_t tp_io = NULL;
    esp_lcd_panel_io_i2c_config_t tp_io_config =
        ESP_LCD_TOUCH_IO_I2C_FT5x06_CONFIG();

    /* v5.5.4 legacy I2C 驱动下不要在 panel_io 配置中设置 scl_speed_hz,
       速度由 bsp_i2c_init 的 master.clk_speed 决定 */
    ESP_RETURN_ON_ERROR(
        esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)s_i2c_port,
                                  &tp_io_config, &tp_io),
        TAG, "Touch IO init failed");

    ESP_RETURN_ON_ERROR(
        esp_lcd_touch_new_i2c_ft5x06(tp_io, &tp_cfg, ret_touch),
        TAG, "Touch driver init failed");

    ESP_LOGI(TAG, "Touch initialized");
    return ESP_OK;
}

#endif /* CONFIG_CAPILOT_BSP_TOUCH_ENABLE */

/******************************************************************************/
/*                        LVGL 集成（条件编译）                                */
/******************************************************************************/

#if CONFIG_CAPILOT_BSP_LVGL_ENABLE

static lv_disp_t *s_disp = NULL;
static lv_indev_t *s_indev = NULL;

static lv_disp_t *bsp_display_lvgl_init(void)
{
    ESP_LOGD(TAG, "Add LVGL display");

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle    = s_io_handle,
        .panel_handle = s_panel_handle,
        .buffer_size  = CAPILOT_BSP_LCD_H_RES * 20,
        .double_buffer = true,
        .hres         = CAPILOT_BSP_LCD_H_RES,
        .vres         = CAPILOT_BSP_LCD_V_RES,
        .monochrome   = false,
        .rotation = {
            .swap_xy  = true,
            .mirror_x = true,
            .mirror_y = false,
        },
        .flags = {
            .buff_dma   = true,
            .buff_spiram = false,
        },
    };

    return lvgl_port_add_disp(&disp_cfg);
}

static lv_indev_t *bsp_indev_lvgl_init(lv_disp_t *disp)
{
    esp_lcd_touch_handle_t tp = NULL;
    ESP_ERROR_CHECK(capilot_bsp_touch_init(&tp));
    assert(tp);

    const lvgl_port_touch_cfg_t touch_cfg = {
        .disp  = disp,
        .handle = tp,
    };
    return lvgl_port_add_touch(&touch_cfg);
}

void capilot_bsp_lvgl_start(void)
{
    ESP_LOGI(TAG, "Starting LVGL...");

    /* LVGL core */
    lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    lvgl_port_init(&lvgl_cfg);

    /* LCD + LVGL display */
    capilot_bsp_lcd_init();
    capilot_bsp_lcd_set_color(0x0000);  /* black background */
    s_disp = bsp_display_lvgl_init();

    /* Touch + LVGL input */
    s_indev = bsp_indev_lvgl_init(s_disp);

    /* Backlight on */
    capilot_bsp_backlight_init();
    capilot_bsp_backlight_on();

    ESP_LOGI(TAG, "LVGL ready");
}

lv_disp_t *capilot_bsp_lvgl_get_display(void)
{
    return s_disp;
}

lv_indev_t *capilot_bsp_lvgl_get_indev(void)
{
    return s_indev;
}

#endif /* CONFIG_CAPILOT_BSP_LVGL_ENABLE */
