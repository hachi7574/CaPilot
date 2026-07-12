#pragma once

/*
 * This header is deliberately injected before esp_tinyusb's default config
 * when compiling the raw TinyUSB component. esp_tinyusb 2.2 does not expose
 * UAC2 through Kconfig, while TinyUSB 0.19 already contains the UAC2 driver.
 */
#include "tusb_option.h"
#include "sdkconfig.h"
#include "esp_attr.h"
#include "esp_rom_sys.h"

#ifndef CONFIG_TINYUSB_HID_COUNT
#define CONFIG_TINYUSB_HID_COUNT 1
#endif
#ifndef CONFIG_TINYUSB_DEBUG_LEVEL
#define CONFIG_TINYUSB_DEBUG_LEVEL 0
#endif

#define CFG_TUD_ENABLED 1
#define CFG_TUD_MAX_SPEED OPT_MODE_FULL_SPEED
#define CFG_TUD_DWC2_SLAVE_ENABLE 1
#define CFG_TUSB_OS OPT_OS_FREERTOS
#define CFG_TUSB_DEBUG CONFIG_TINYUSB_DEBUG_LEVEL
#define CFG_TUSB_DEBUG_PRINTF esp_rom_printf
#define CFG_TUSB_MEM_SECTION DRAM_ATTR
#define CFG_TUSB_MEM_ALIGN TU_ATTR_ALIGNED(4)
#define CFG_TUD_ENDPOINT0_SIZE 64

#define CFG_TUD_CDC 0
#define CFG_TUD_MSC 0
#define CFG_TUD_HID CONFIG_TINYUSB_HID_COUNT
#define CFG_TUD_MIDI 0
#define CFG_TUD_VENDOR 0
#define CFG_TUD_ECM_RNDIS 0
#define CFG_TUD_NCM 0
#define CFG_TUD_DFU 0
#define CFG_TUD_DFU_RUNTIME 0
#define CFG_TUD_BTH 0

/* UAC1: 48 kHz, 16-bit, stereo speaker (RX) plus mono microphone (TX). */
#define CFG_TUD_AUDIO 1
#define CFG_TUD_AUDIO_FUNC_1_DESC_LEN 183
#define CFG_TUD_AUDIO_FUNC_1_CTRL_BUF_SZ 64
#define CFG_TUD_AUDIO_FUNC_1_N_FORMATS 1
#define CFG_TUD_AUDIO_FUNC_1_MAX_SAMPLE_RATE 48000
#define CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX 1
#define CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX 2
#define CFG_TUD_AUDIO_FUNC_1_FORMAT_1_N_BYTES_PER_SAMPLE_TX 2
#define CFG_TUD_AUDIO_FUNC_1_FORMAT_1_RESOLUTION_TX 16
#define CFG_TUD_AUDIO_FUNC_1_FORMAT_1_N_BYTES_PER_SAMPLE_RX 2
#define CFG_TUD_AUDIO_FUNC_1_FORMAT_1_RESOLUTION_RX 16
#define CFG_TUD_AUDIO_ENABLE_EP_IN 1
#define CFG_TUD_AUDIO_ENABLE_EP_OUT 1
#define CFG_TUD_AUDIO_ENABLE_INTERRUPT_EP 0
#define CFG_TUD_AUDIO_FUNC_1_EP_IN_SZ_MAX 96
#define CFG_TUD_AUDIO_FUNC_1_EP_OUT_SZ_MAX 192
#define CFG_TUD_AUDIO_FUNC_1_EP_IN_SW_BUF_SZ 384
#define CFG_TUD_AUDIO_FUNC_1_EP_OUT_SW_BUF_SZ 768
