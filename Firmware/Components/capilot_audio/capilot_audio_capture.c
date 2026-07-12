/**
 * @file capilot_audio_capture.c
 * @brief CaPilot Audio 组件 — 音频采集实现（ES7210）
 *
 * 硬件连接：
 *   ES7210 I2C 地址: 0x41 (AD0=1, AD1=0)
 *   I2S STD 全双工: MCK=GPIO38, BCK=GPIO14, WS=GPIO13, DI=GPIO12
 *   ES7210 作为 I2S master 创建 RX+TX 全双工 channel（I2S0），
 *   TX handle 通过 capilot_audio_get_shared_tx_chan() 暴露给 ES8311 复用。
 *
 * 寄存器初始化序列提取自乐鑫 es7210 组件库 v1.0.1
 * (https://github.com/espressif/esp-bsp/tree/master/components/es7210)
 * 适配为使用 BSP I2C 读写函数（新版 i2c_master API）。
 */

#include "capilot_audio_capture.h"
#include "capilot_audio_driver.h"
#include "capilot_audio_internal.h"
#include "capilot_bsp.h"
#include "esp_log.h"
#include "esp_check.h"
#include "esp_timer.h"
#include "driver/i2s_std.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "capilot_audio_cap";

/* ============ ES7210 硬件参数 ============ */
#define ES7210_I2C_ADDR         0x41
#define ES7210_I2S_MCK_IO       GPIO_NUM_38
#define ES7210_I2S_BCK_IO       GPIO_NUM_14
#define ES7210_I2S_WS_IO        GPIO_NUM_13
#define ES7210_I2S_DI_IO        GPIO_NUM_12
#define ES8311_I2S_DO_IO        GPIO_NUM_45

/* ============ ES7210 寄存器地址 ============ */
#define ES7210_RESET_REG00          0x00
#define ES7210_MAINCLK_REG02        0x02
#define ES7210_LRCK_DIVH_REG04      0x04
#define ES7210_LRCK_DIVL_REG05      0x05
#define ES7210_POWER_DOWN_REG06     0x06
#define ES7210_OSR_REG07            0x07
#define ES7210_TIME_CONTROL0_REG09  0x09
#define ES7210_TIME_CONTROL1_REG0A  0x0A
#define ES7210_ADC34_HPF2_REG20     0x20
#define ES7210_ADC34_HPF1_REG21     0x21
#define ES7210_ADC12_HPF2_REG22     0x22
#define ES7210_ADC12_HPF1_REG23     0x23
#define ES7210_SDP_INTERFACE1_REG11 0x11
#define ES7210_SDP_INTERFACE2_REG12 0x12
#define ES7210_ANALOG_REG40         0x40
#define ES7210_MIC12_BIAS_REG41     0x41
#define ES7210_MIC34_BIAS_REG42     0x42
#define ES7210_MIC1_GAIN_REG43      0x43
#define ES7210_MIC2_GAIN_REG44      0x44
#define ES7210_MIC3_GAIN_REG45      0x45
#define ES7210_MIC4_GAIN_REG46      0x46
#define ES7210_MIC1_POWER_REG47     0x47
#define ES7210_MIC2_POWER_REG48     0x48
#define ES7210_MIC3_POWER_REG49     0x49
#define ES7210_MIC4_POWER_REG4A     0x4A
#define ES7210_MIC12_POWER_REG4B    0x4B
#define ES7210_MIC34_POWER_REG4C    0x4C

/* ============ 时钟系数表（提取自 es7210 组件库） ============ */
typedef struct {
    uint32_t mclk;
    uint32_t lrck;
    uint8_t  adc_div;
    uint8_t  dll;
    uint8_t  doubler;
    uint8_t  osr;
    uint8_t  lrck_h;
    uint8_t  lrck_l;
} es7210_coeff_t;

static const es7210_coeff_t s_coeff_table[] = {
    /* 16kHz */
    { 4096000,  16000, 0x01, 0x01, 0x01, 0x20, 0x01, 0x00 },
    {12288000,  16000, 0x03, 0x01, 0x01, 0x20, 0x03, 0x00 },
    /* 48kHz */
    {12288000,  48000, 0x01, 0x01, 0x01, 0x20, 0x01, 0x00 },
};

/* ============ 内部状态 ============ */
static bool s_initialized = false;
static bool s_running = false;
static capilot_audio_capture_callback_t s_callback = NULL;
static i2s_chan_handle_t s_rx_chan = NULL;
static i2s_chan_handle_t s_tx_chan = NULL;  /* 全双工 TX handle，供 ES8311 复用 */
static capilot_audio_config_t s_config;

/* ============ I2C 读写辅助 ============ */

static esp_err_t es7210_write(uint8_t reg, uint8_t val)
{
    return capilot_bsp_i2c_write_byte(ES7210_I2C_ADDR, reg, val);
}

static esp_err_t es7210_read(uint8_t reg, uint8_t *val)
{
    return capilot_bsp_i2c_read(ES7210_I2C_ADDR, reg, val, 1);
}

/* ============ ES7210 寄存器初始化 ============ */

static esp_err_t es7210_set_sample_rate(uint32_t sample_rate, uint32_t mclk_ratio)
{
    uint32_t mclk = sample_rate * mclk_ratio;
    const es7210_coeff_t *coeff = NULL;

    for (int i = 0; i < sizeof(s_coeff_table) / sizeof(s_coeff_table[0]); i++) {
        if (s_coeff_table[i].lrck == sample_rate && s_coeff_table[i].mclk == mclk) {
            coeff = &s_coeff_table[i];
            break;
        }
    }
    if (coeff == NULL) {
        ESP_LOGE(TAG, "unsupported sample rate: %luHz mclk: %luHz",
                 (unsigned long)sample_rate, (unsigned long)mclk);
        return ESP_ERR_NOT_SUPPORTED;
    }

    ESP_RETURN_ON_ERROR(es7210_write(ES7210_OSR_REG07, coeff->osr), TAG, "write OSR");
    ESP_RETURN_ON_ERROR(es7210_write(ES7210_MAINCLK_REG02,
        (coeff->adc_div) | (coeff->doubler << 6) | (coeff->dll << 7)), TAG, "write MAINCLK");
    ESP_RETURN_ON_ERROR(es7210_write(ES7210_LRCK_DIVH_REG04, coeff->lrck_h), TAG, "write LRCK_H");
    ESP_RETURN_ON_ERROR(es7210_write(ES7210_LRCK_DIVL_REG05, coeff->lrck_l), TAG, "write LRCK_L");

    return ESP_OK;
}

static esp_err_t es7210_init_regs(const capilot_audio_config_t *config)
{
    /* 采样率→MCLK ratio 映射（256 是标准值） */
    uint32_t mclk_ratio = 256;

    /* 1. 软件复位 */
    ESP_RETURN_ON_ERROR(es7210_write(ES7210_RESET_REG00, 0xFF), TAG, "reset");
    ESP_RETURN_ON_ERROR(es7210_write(ES7210_RESET_REG00, 0x32), TAG, "exit reset");

    /* 2. 初始化时间控制 */
    ESP_RETURN_ON_ERROR(es7210_write(ES7210_TIME_CONTROL0_REG09, 0x30), TAG, "time ctrl0");
    ESP_RETURN_ON_ERROR(es7210_write(ES7210_TIME_CONTROL1_REG0A, 0x30), TAG, "time ctrl1");

    /* 3. HPF 配置（高通滤波器） */
    ESP_RETURN_ON_ERROR(es7210_write(ES7210_ADC12_HPF1_REG23, 0x2A), TAG, "ADC12 HPF1");
    ESP_RETURN_ON_ERROR(es7210_write(ES7210_ADC12_HPF2_REG22, 0x0A), TAG, "ADC12 HPF2");
    ESP_RETURN_ON_ERROR(es7210_write(ES7210_ADC34_HPF1_REG21, 0x2A), TAG, "ADC34 HPF1");
    ESP_RETURN_ON_ERROR(es7210_write(ES7210_ADC34_HPF2_REG20, 0x0A), TAG, "ADC34 HPF2");

    /* 4. SDP 接口：I2S 格式 + 16bit（标准 I2S，非 TDM） */
    ESP_RETURN_ON_ERROR(es7210_write(ES7210_SDP_INTERFACE1_REG11, 0x60), TAG, "SDP1"); /* I2S + 16bit */
    ESP_RETURN_ON_ERROR(es7210_write(ES7210_SDP_INTERFACE2_REG12, 0x00), TAG, "SDP2"); /* TDM 禁止 */

    /* 5. 模拟功率和 VMID 电压 */
    ESP_RETURN_ON_ERROR(es7210_write(ES7210_ANALOG_REG40, 0xC3), TAG, "analog");

    /* 6. MIC 偏置 2.87V */
    ESP_RETURN_ON_ERROR(es7210_write(ES7210_MIC12_BIAS_REG41, 0x70), TAG, "MIC12 bias");
    ESP_RETURN_ON_ERROR(es7210_write(ES7210_MIC34_BIAS_REG42, 0x70), TAG, "MIC34 bias");

    /* 7. MIC 增益 30dB */
    uint8_t gain = 0x0A; /* ES7210_MIC_GAIN_30DB */
    ESP_RETURN_ON_ERROR(es7210_write(ES7210_MIC1_GAIN_REG43, gain | 0x10), TAG, "MIC1 gain");
    ESP_RETURN_ON_ERROR(es7210_write(ES7210_MIC2_GAIN_REG44, gain | 0x10), TAG, "MIC2 gain");
    ESP_RETURN_ON_ERROR(es7210_write(ES7210_MIC3_GAIN_REG45, gain | 0x10), TAG, "MIC3 gain");
    ESP_RETURN_ON_ERROR(es7210_write(ES7210_MIC4_GAIN_REG46, gain | 0x10), TAG, "MIC4 gain");

    /* 8. MIC 上电 */
    ESP_RETURN_ON_ERROR(es7210_write(ES7210_MIC1_POWER_REG47, 0x08), TAG, "MIC1 power");
    ESP_RETURN_ON_ERROR(es7210_write(ES7210_MIC2_POWER_REG48, 0x08), TAG, "MIC2 power");
    ESP_RETURN_ON_ERROR(es7210_write(ES7210_MIC3_POWER_REG49, 0x08), TAG, "MIC3 power");
    ESP_RETURN_ON_ERROR(es7210_write(ES7210_MIC4_POWER_REG4A, 0x08), TAG, "MIC4 power");

    /* 9. 采样率配置 */
    ESP_RETURN_ON_ERROR(es7210_set_sample_rate(config->sample_rate, mclk_ratio), TAG, "sample rate");

    /* 10. 关闭 DLL */
    ESP_RETURN_ON_ERROR(es7210_write(ES7210_POWER_DOWN_REG06, 0x04), TAG, "power down DLL");

    /* 11. MIC 偏置 & ADC & PGA 上电 */
    ESP_RETURN_ON_ERROR(es7210_write(ES7210_MIC12_POWER_REG4B, 0x0F), TAG, "MIC12 power");
    ESP_RETURN_ON_ERROR(es7210_write(ES7210_MIC34_POWER_REG4C, 0x0F), TAG, "MIC34 power");

    /* 12. 使能设备 */
    ESP_RETURN_ON_ERROR(es7210_write(ES7210_RESET_REG00, 0x71), TAG, "enable step1");
    ESP_RETURN_ON_ERROR(es7210_write(ES7210_RESET_REG00, 0x41), TAG, "enable step2");

    return ESP_OK;
}

/* ============ I2S STD 全双工初始化 ============ */

static esp_err_t es7210_i2s_init(uint32_t sample_rate)
{
    ESP_LOGI(TAG, "create I2S STD full-duplex channel (RX+TX on I2S0)");

    /* 全双工：同时创建 RX 和 TX channel，共享 I2S0 控制器
     * 注意：i2s_new_channel 参数顺序是 (cfg, tx, rx) */
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    ESP_RETURN_ON_ERROR(i2s_new_channel(&chan_cfg, &s_tx_chan, &s_rx_chan), TAG, "new channel");

    /* RX/TX full-duplex shares the same I2S0 STD GPIO matrix.
     * Configure ES8311 DOUT here as well; otherwise later TX-only init can
     * succeed in software while the physical data pin is left unbound. */
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(sample_rate),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(
            I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = ES7210_I2S_MCK_IO,
            .bclk = ES7210_I2S_BCK_IO,
            .ws   = ES7210_I2S_WS_IO,
            .dout = ES8311_I2S_DO_IO,
            .din  = ES7210_I2S_DI_IO,
        },
    };
    std_cfg.clk_cfg.mclk_multiple = I2S_MCLK_MULTIPLE_256;

    ESP_RETURN_ON_ERROR(i2s_channel_init_std_mode(s_rx_chan, &std_cfg), TAG, "init rx std");
    /* TX channel 不在此初始化，由 ES8311 播放驱动用 capilot_audio_get_shared_tx_chan() 获取后初始化 */
    return ESP_OK;
}

/* 暴露共享 TX channel 给 ES8311 播放驱动复用 */
i2s_chan_handle_t capilot_audio_get_shared_tx_chan(void)
{
    return s_tx_chan;
}

/* 暴露共享 RX channel，播放时需保持 RX enable 以维持控制器时钟 */
i2s_chan_handle_t capilot_audio_get_shared_rx_chan(void)
{
    return s_rx_chan;
}

/* ============ ES7210 驱动 vtable ============ */

static bool es7210_is_present(void)
{
    /* 尝试读复位寄存器，能读到说明芯片在线 */
    uint8_t val = 0;
    return (es7210_read(ES7210_RESET_REG00, &val) == ESP_OK);
}

static esp_err_t es7210_init_cb(const capilot_audio_config_t *config)
{
    if (!es7210_is_present()) {
        return ESP_ERR_NOT_FOUND;
    }
    s_config = *config;
    ESP_RETURN_ON_ERROR(es7210_init_regs(config), TAG, "init regs");
    ESP_RETURN_ON_ERROR(es7210_i2s_init(config->sample_rate), TAG, "i2s init");
    ESP_LOGI(TAG, "ES7210 initialized: %luHz %dbit",
             (unsigned long)config->sample_rate, config->bits);
    return ESP_OK;
}

static esp_err_t es7210_config_cb(const capilot_audio_config_t *config)
{
    /* 运行时重新配置采样率 */
    s_config = *config;
    ESP_RETURN_ON_ERROR(es7210_set_sample_rate(config->sample_rate, 256), TAG, "set sample rate");
    return ESP_OK;
}

static esp_err_t es7210_start_cb(void)
{
    if (s_rx_chan == NULL) return ESP_ERR_INVALID_STATE;
    return i2s_channel_enable(s_rx_chan);
}

static esp_err_t es7210_stop_cb(void)
{
    if (s_rx_chan == NULL) return ESP_ERR_INVALID_STATE;
    return i2s_channel_disable(s_rx_chan);
}

static esp_err_t es7210_read_cb(capilot_audio_buffer_t *buffer)
{
    if (s_rx_chan == NULL || buffer == NULL || buffer->data == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    size_t bytes_read = 0;
    esp_err_t ret = i2s_channel_read(s_rx_chan, buffer->data, buffer->length,
                                     &bytes_read, pdMS_TO_TICKS(100));
    if (ret == ESP_OK) {
        buffer->length = bytes_read;
        buffer->timestamp = esp_timer_get_time();
    }
    return ret;
}

static esp_err_t es7210_write_cb(const capilot_audio_buffer_t *buffer)
{
    return ESP_ERR_NOT_SUPPORTED; /* 采集驱动不支持写入 */
}

const capilot_audio_driver_t s_es7210_driver = {
    .name      = "es7210",
    .init      = es7210_init_cb,
    .config    = es7210_config_cb,
    .start     = es7210_start_cb,
    .stop      = es7210_stop_cb,
    .read      = es7210_read_cb,
    .write     = es7210_write_cb,
    .is_present = es7210_is_present,
};

/* ============ 公共 API 实现 ============ */

#define CAP_READ_BUF_SIZE  4096

esp_err_t capilot_audio_capture_init(void)
{
    if (s_initialized) return ESP_OK;
    s_initialized = true;
    ESP_LOGI(TAG, "audio capture initialized");
    return ESP_OK;
}

esp_err_t capilot_audio_capture_start(void)
{
    if (!s_initialized) {
        ESP_RETURN_ON_ERROR(capilot_audio_capture_init(), TAG, "init");
    }
    if (s_running) return ESP_OK;

    if (s_rx_chan != NULL) {
        ESP_RETURN_ON_ERROR(i2s_channel_enable(s_rx_chan), TAG, "enable");
    }
    s_running = true;
    ESP_LOGI(TAG, "audio capture started");
    return ESP_OK;
}

esp_err_t capilot_audio_capture_stop(void)
{
    if (!s_running) return ESP_OK;

    if (s_rx_chan != NULL) {
        i2s_channel_disable(s_rx_chan);
    }
    s_running = false;
    ESP_LOGI(TAG, "audio capture stopped");
    return ESP_OK;
}

bool capilot_audio_capture_is_running(void)
{
    return s_running;
}

esp_err_t capilot_audio_capture_read(capilot_audio_buffer_t *buffer)
{
    if (!s_running || buffer == NULL || buffer->data == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    size_t bytes_read = 0;
    esp_err_t ret = i2s_channel_read(s_rx_chan, buffer->data, buffer->length,
                                     &bytes_read, pdMS_TO_TICKS(100));
    if (ret == ESP_OK) {
        buffer->length = bytes_read;
        buffer->timestamp = esp_timer_get_time();
    }
    return ret;
}

esp_err_t capilot_audio_capture_set_callback(capilot_audio_capture_callback_t callback)
{
    s_callback = callback;
    /* TODO: 如果设置了回调，启动 I2S 接收任务 */
    return ESP_OK;
}
