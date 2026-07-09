/**
 * @file capilot_audio_playback.c
 * @brief CaPilot Audio 组件 — 音频播放实现（ES8311）
 *
 * 硬件连接：
 *   ES8311 I2C 地址: 0x18 (CE=0)
 *   I2S STD 模式: MCK=GPIO38, BCK=GPIO14, WS=GPIO13, DO=GPIO45
 *   音频功放: PCA9557 IO1 (通过 BSP 控制)
 *
 * 寄存器初始化序列提取自乐鑫 es8311 组件库 v1.0.0
 * (https://github.com/espressif/esp-bsp/tree/master/components/es8311)
 * 适配为使用 BSP I2C 读写函数（新版 i2c_master API）。
 */

#include "capilot_audio_playback.h"
#include "capilot_audio_driver.h"
#include "capilot_audio_internal.h"
#include "capilot_bsp.h"
#include "esp_log.h"
#include "esp_check.h"
#include "driver/i2s_std.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

static const char *TAG = "capilot_audio_pb";

/* ============ ES8311 硬件参数 ============ */
#define ES8311_I2C_ADDR         0x18
#define ES8311_I2S_MCK_IO       GPIO_NUM_38
#define ES8311_I2S_BCK_IO       GPIO_NUM_14
#define ES8311_I2S_WS_IO        GPIO_NUM_13
#define ES8311_I2S_DO_IO        GPIO_NUM_45
#define ES8311_I2S_DI_IO        (-1)   /* 不用 ES8311 的 ADC */

/* ============ ES8311 寄存器地址 ============ */
#define ES8311_RESET_REG00          0x00
#define ES8311_CLK_MANAGER_REG01    0x01
#define ES8311_CLK_MANAGER_REG02    0x02
#define ES8311_CLK_MANAGER_REG03    0x03
#define ES8311_CLK_MANAGER_REG04    0x04
#define ES8311_CLK_MANAGER_REG05    0x05
#define ES8311_CLK_MANAGER_REG06    0x06
#define ES8311_CLK_MANAGER_REG07    0x07
#define ES8311_CLK_MANAGER_REG08    0x08
#define ES8311_SDPIN_REG09          0x09
#define ES8311_SDPOUT_REG0A         0x0A
#define ES8311_SYSTEM_REG0D         0x0D
#define ES8311_SYSTEM_REG0E         0x0E
#define ES8311_SYSTEM_REG12         0x12
#define ES8311_SYSTEM_REG13         0x13
#define ES8311_SYSTEM_REG14         0x14
#define ES8311_ADC_REG17            0x17
#define ES8311_ADC_REG1C            0x1C
#define ES8311_DAC_REG31            0x31
#define ES8311_DAC_REG32            0x32
#define ES8311_DAC_REG37            0x37

/* ============ 时钟系数表（提取自 es8311 组件库） ============ */
typedef struct {
    uint32_t mclk;
    uint32_t lrck;
    uint8_t  pre_div;
    uint8_t  pre_multi;
    uint8_t  fs_mode;   /* 0=单速, 1=双速 */
    uint8_t  adc_osr;
    uint8_t  dac_osr;
    uint8_t  adc_div;
    uint8_t  dac_div;
    uint8_t  bclk_div;
    uint8_t  lrck_h;
    uint8_t  lrck_l;
} es8311_coeff_t;

/* 16bit, MCLK=256*Fs 的常用配置 */
static const es8311_coeff_t s_coeff_table[] = {
    /* 16kHz, MCLK=4096000 */
    { 4096000, 16000, 1, 0, 0, 0x10, 0x10, 1, 1, 4, 0x00, 0x80 },
    /* 48kHz, MCLK=12288000 */
    {12288000, 48000, 1, 0, 0, 0x10, 0x10, 1, 1, 4, 0x00, 0xC0 },
};

/* ============ 内部状态 ============ */
static bool s_initialized = false;
static bool s_running = false;
static i2s_chan_handle_t s_tx_chan = NULL;
static int s_volume = 70;

/* ============ I2C 读写辅助 ============ */

static esp_err_t es8311_write(uint8_t reg, uint8_t val)
{
    return capilot_bsp_i2c_write_byte(ES8311_I2C_ADDR, reg, val);
}

static esp_err_t es8311_read(uint8_t reg, uint8_t *val)
{
    return capilot_bsp_i2c_read(ES8311_I2C_ADDR, reg, val, 1);
}

/* ============ ES8311 寄存器初始化 ============ */

static esp_err_t es8311_set_sample_rate(uint32_t sample_rate, uint32_t mclk_ratio)
{
    uint32_t mclk = sample_rate * mclk_ratio;
    const es8311_coeff_t *coeff = NULL;

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

    uint8_t val;

    /* REG02: 预分频器 */
    ESP_RETURN_ON_ERROR(es8311_read(ES8311_CLK_MANAGER_REG02, &val), TAG, "read reg02");
    ESP_RETURN_ON_ERROR(es8311_write(ES8311_CLK_MANAGER_REG02,
        (val & 0x07) | ((coeff->pre_div - 1) << 5) | (coeff->pre_multi << 3)), TAG, "write reg02");

    /* REG03: 采样率模式 + ADC OSR */
    ESP_RETURN_ON_ERROR(es8311_write(ES8311_CLK_MANAGER_REG03,
        (coeff->fs_mode << 6) | coeff->adc_osr), TAG, "write reg03");

    /* REG04: DAC OSR */
    ESP_RETURN_ON_ERROR(es8311_write(ES8311_CLK_MANAGER_REG04, coeff->dac_osr), TAG, "write reg04");

    /* REG05: ADC/DAC 分频器 */
    ESP_RETURN_ON_ERROR(es8311_write(ES8311_CLK_MANAGER_REG05,
        ((coeff->adc_div - 1) << 4) | (coeff->dac_div - 1)), TAG, "write reg05");

    /* REG06: SCLK 分频器 */
    ESP_RETURN_ON_ERROR(es8311_read(ES8311_CLK_MANAGER_REG06, &val), TAG, "read reg06");
    ESP_RETURN_ON_ERROR(es8311_write(ES8311_CLK_MANAGER_REG06,
        (val & 0xE0) | (coeff->bclk_div - 1)), TAG, "write reg06");

    /* REG07/08: LRCK 分频器 */
    ESP_RETURN_ON_ERROR(es8311_read(ES8311_CLK_MANAGER_REG07, &val), TAG, "read reg07");
    ESP_RETURN_ON_ERROR(es8311_write(ES8311_CLK_MANAGER_REG07,
        (val & 0xC0) | coeff->lrck_h), TAG, "write reg07");
    ESP_RETURN_ON_ERROR(es8311_write(ES8311_CLK_MANAGER_REG08, coeff->lrck_l), TAG, "write reg08");

    return ESP_OK;
}

static esp_err_t es8311_init_regs(const capilot_audio_config_t *config)
{
    uint32_t mclk_ratio = 256;

    /* 1. 复位 */
    ESP_RETURN_ON_ERROR(es8311_write(ES8311_RESET_REG00, 0x1F), TAG, "reset");
    vTaskDelay(pdMS_TO_TICKS(20));
    ESP_RETURN_ON_ERROR(es8311_write(ES8311_RESET_REG00, 0x00), TAG, "exit reset");
    ESP_RETURN_ON_ERROR(es8311_write(ES8311_RESET_REG00, 0x80), TAG, "power on");

    /* 2. 使能所有时钟，MCLK 来自 MCLK 引脚 */
    ESP_RETURN_ON_ERROR(es8311_write(ES8311_CLK_MANAGER_REG01, 0x3F), TAG, "clk manager");

    /* 3. SCLK 不反转 */
    uint8_t val;
    ESP_RETURN_ON_ERROR(es8311_read(ES8311_CLK_MANAGER_REG06, &val), TAG, "read reg06");
    ESP_RETURN_ON_ERROR(es8311_write(ES8311_CLK_MANAGER_REG06, val & ~BIT(5)), TAG, "sclk polarity");

    /* 4. 采样率配置 */
    ESP_RETURN_ON_ERROR(es8311_set_sample_rate(config->sample_rate, mclk_ratio), TAG, "sample rate");

    /* 5. 从机模式 */
    ESP_RETURN_ON_ERROR(es8311_read(ES8311_RESET_REG00, &val), TAG, "read reg00");
    ESP_RETURN_ON_ERROR(es8311_write(ES8311_RESET_REG00, val & 0xBF), TAG, "slave mode");

    /* 6. SDP 格式：16bit */
    ESP_RETURN_ON_ERROR(es8311_write(ES8311_SDPIN_REG09, 0x0C), TAG, "SDP in 16bit");
    ESP_RETURN_ON_ERROR(es8311_write(ES8311_SDPOUT_REG0A, 0x0C), TAG, "SDP out 16bit");

    /* 7. 上电模拟电路 */
    ESP_RETURN_ON_ERROR(es8311_write(ES8311_SYSTEM_REG0D, 0x01), TAG, "analog power");

    /* 8. 使能 PGA & ADC */
    ESP_RETURN_ON_ERROR(es8311_write(ES8311_SYSTEM_REG0E, 0x02), TAG, "PGA ADC");

    /* 9. 上电 DAC */
    ESP_RETURN_ON_ERROR(es8311_write(ES8311_SYSTEM_REG12, 0x00), TAG, "DAC power");

    /* 10. 使能耳机输出 */
    ESP_RETURN_ON_ERROR(es8311_write(ES8311_SYSTEM_REG13, 0x10), TAG, "headphone");

    /* 11. ADC EQ 旁路 + DC 偏移消除 */
    ESP_RETURN_ON_ERROR(es8311_write(ES8311_ADC_REG1C, 0x6A), TAG, "ADC EQ");

    /* 12. DAC EQ 旁路 */
    ESP_RETURN_ON_ERROR(es8311_write(ES8311_DAC_REG37, 0x08), TAG, "DAC EQ");

    /* 13. 麦克风配置（模拟麦克风，禁用数字麦克风） */
    ESP_RETURN_ON_ERROR(es8311_write(ES8311_ADC_REG17, 0xC8), TAG, "ADC gain");
    ESP_RETURN_ON_ERROR(es8311_write(ES8311_SYSTEM_REG14, 0x1A), TAG, "mic config");

    /* 14. 设置音量 */
    uint8_t vol_reg = (s_volume == 0) ? 0x00 : (uint8_t)((s_volume * 256 / 100) - 1);
    ESP_RETURN_ON_ERROR(es8311_write(ES8311_DAC_REG32, vol_reg), TAG, "volume");

    return ESP_OK;
}

/* ============ I2S STD 初始化（复用全双工 TX channel） ============ */

static esp_err_t es8311_i2s_init(uint32_t sample_rate)
{
    /* 复用 ES7210 创建的全双工 TX channel（I2S0），避免两个控制器争用引脚 */
    s_tx_chan = capilot_audio_get_shared_tx_chan();
    if (s_tx_chan != NULL) {
        ESP_LOGI(TAG, "reuse shared I2S0 tx channel (full-duplex)");
    } else {
        /* Fallback: ES7210 不在线时独立创建 TX channel（I2S0 单工） */
        ESP_LOGI(TAG, "create standalone I2S0 tx channel (ES7210 not present)");
        i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
        chan_cfg.auto_clear = true;
        ESP_RETURN_ON_ERROR(i2s_new_channel(&chan_cfg, &s_tx_chan, NULL), TAG, "new channel");
    }

    /* TX 初始化为 STD 模式，只配置 dout（mclk/bclk/ws 已由 RX 配置） */
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(sample_rate),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(
            I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = (s_tx_chan == capilot_audio_get_shared_tx_chan()) ? -1 : ES8311_I2S_MCK_IO,
            .bclk = (s_tx_chan == capilot_audio_get_shared_tx_chan()) ? -1 : ES8311_I2S_BCK_IO,
            .ws   = (s_tx_chan == capilot_audio_get_shared_tx_chan()) ? -1 : ES8311_I2S_WS_IO,
            .dout = ES8311_I2S_DO_IO,
            .din  = ES8311_I2S_DI_IO,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };
    std_cfg.clk_cfg.mclk_multiple = I2S_MCLK_MULTIPLE_256;

    ESP_RETURN_ON_ERROR(i2s_channel_init_std_mode(s_tx_chan, &std_cfg), TAG, "init std tx");
    return ESP_OK;
}

/* ============ ES8311 驱动 vtable ============ */

static bool es8311_is_present(void)
{
    uint8_t val = 0;
    return (es8311_read(ES8311_RESET_REG00, &val) == ESP_OK);
}

static esp_err_t es8311_init_cb(const capilot_audio_config_t *config)
{
    if (!es8311_is_present()) {
        return ESP_ERR_NOT_FOUND;
    }
    ESP_RETURN_ON_ERROR(es8311_init_regs(config), TAG, "init regs");
    ESP_RETURN_ON_ERROR(es8311_i2s_init(config->sample_rate), TAG, "i2s init");

    /* 使能音频功放 */
    capilot_bsp_pa_enable(true);

    ESP_LOGI(TAG, "ES8311 initialized: %luHz %dbit vol=%d",
             (unsigned long)config->sample_rate, config->bits, s_volume);
    return ESP_OK;
}

static esp_err_t es8311_config_cb(const capilot_audio_config_t *config)
{
    return es8311_set_sample_rate(config->sample_rate, 256);
}

static esp_err_t es8311_start_cb(void)
{
    if (s_tx_chan == NULL) return ESP_ERR_INVALID_STATE;
    return i2s_channel_enable(s_tx_chan);
}

static esp_err_t es8311_stop_cb(void)
{
    if (s_tx_chan == NULL) return ESP_ERR_INVALID_STATE;
    return i2s_channel_disable(s_tx_chan);
}

static esp_err_t es8311_read_cb(capilot_audio_buffer_t *buffer)
{
    return ESP_ERR_NOT_SUPPORTED; /* 播放驱动不支持读取 */
}

static esp_err_t es8311_write_cb(const capilot_audio_buffer_t *buffer)
{
    if (s_tx_chan == NULL || buffer == NULL || buffer->data == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    size_t bytes_write = 0;
    esp_err_t ret = i2s_channel_write(s_tx_chan, buffer->data, buffer->length,
                                      &bytes_write, pdMS_TO_TICKS(100));
    return ret;
}

const capilot_audio_driver_t s_es8311_driver = {
    .name      = "es8311",
    .init      = es8311_init_cb,
    .config    = es8311_config_cb,
    .start     = es8311_start_cb,
    .stop      = es8311_stop_cb,
    .read      = es8311_read_cb,
    .write     = es8311_write_cb,
    .is_present = es8311_is_present,
};

/* ============ 公共 API 实现 ============ */

esp_err_t capilot_audio_playback_init(void)
{
    if (s_initialized) return ESP_OK;
    s_initialized = true;
    ESP_LOGI(TAG, "audio playback initialized");
    return ESP_OK;
}

esp_err_t capilot_audio_playback_start(void)
{
    if (!s_initialized) {
        ESP_RETURN_ON_ERROR(capilot_audio_playback_init(), TAG, "init");
    }
    if (s_running) return ESP_OK;

    /* 全双工模式：播放前需保持 RX channel enable，否则 I2S 控制器时钟
     * 会随 RX disable 而停止，导致 TX write 超时 */
    i2s_chan_handle_t rx = capilot_audio_get_shared_rx_chan();
    if (rx != NULL) {
        esp_err_t ret = i2s_channel_enable(rx);
        if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
            ESP_LOGW(TAG, "rx keep-enable: %s", esp_err_to_name(ret));
        }
    }

    if (s_tx_chan != NULL) {
        ESP_RETURN_ON_ERROR(i2s_channel_enable(s_tx_chan), TAG, "enable");
    }
    s_running = true;
    ESP_LOGI(TAG, "audio playback started");
    return ESP_OK;
}

esp_err_t capilot_audio_playback_stop(void)
{
    if (!s_running) return ESP_OK;

    if (s_tx_chan != NULL) {
        i2s_channel_disable(s_tx_chan);
    }
    s_running = false;
    ESP_LOGI(TAG, "audio playback stopped");
    return ESP_OK;
}

esp_err_t capilot_audio_playback_write(const capilot_audio_buffer_t *buffer)
{
    if (!s_running || buffer == NULL || buffer->data == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    size_t bytes_write = 0;
    return i2s_channel_write(s_tx_chan, buffer->data, buffer->length,
                             &bytes_write, pdMS_TO_TICKS(100));
}

esp_err_t capilot_audio_playback_beep(uint16_t frequency, uint16_t duration_ms)
{
    if (!s_running || s_tx_chan == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    /* 生成方波数据：16bit stereo, 采样率取当前配置 */
    uint32_t sample_rate = s_coeff_table[0].lrck; /* 简化：用表第一项的采样率 */
    uint32_t samples = (uint32_t)sample_rate * duration_ms / 1000;
    uint32_t buf_size = samples * 4; /* 16bit stereo = 4 bytes/sample */
    int16_t *buf = heap_caps_malloc(buf_size, MALLOC_CAP_DMA);
    if (buf == NULL) return ESP_ERR_NO_MEM;

    uint32_t period_samples = sample_rate / frequency;
    int16_t amp = 8000; /* 避免满幅失真 */
    for (uint32_t i = 0; i < samples; i++) {
        int16_t val = ((i / period_samples) % 2) ? amp : -amp;
        buf[i * 2]     = val; /* L */
        buf[i * 2 + 1] = val; /* R */
    }

    size_t bytes_write = 0;
    esp_err_t ret = i2s_channel_write(s_tx_chan, buf, buf_size,
                                      &bytes_write, pdMS_TO_TICKS(2000));
    free(buf);
    return ret;
}

esp_err_t capilot_audio_playback_wait_done(uint32_t timeout_ms)
{
    /* ESP-IDF v5.x I2S 驱动没有直接的 wait_done API，用延时近似 */
    if (timeout_ms > 0) {
        vTaskDelay(pdMS_TO_TICKS(timeout_ms));
    }
    return ESP_OK;
}
