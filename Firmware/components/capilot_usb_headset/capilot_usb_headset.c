#include "capilot_usb_headset.h"

#include <inttypes.h>
#include <math.h>
#include <string.h>

#include "tinyusb.h"
#include "tinyusb_default_config.h"
#include "tusb.h"
#include "class/audio/audio_device.h"
#include "class/hid/hid_device.h"
#include "esp_log.h"

static const char *TAG = "[usb_headset]";

#define USB_VID_CAPPILOT 0x303A
#define USB_PID_HEADSET  0x4012
#define USB_EP_AUDIO_OUT 0x01
#define USB_EP_AUDIO_IN  0x81
#define USB_EP_HID_IN    0x82

#define UAC1_REQ_SET_CUR 0x01
#define UAC1_REQ_GET_CUR 0x81
#define UAC1_REQ_GET_MIN 0x82
#define UAC1_REQ_GET_MAX 0x83
#define UAC1_REQ_GET_RES 0x84
#define UAC1_EP_CTRL_SAMPLING_FREQ 0x01

enum {
    ITF_AUDIO_CONTROL = 0,
    ITF_AUDIO_SPEAKER,
    ITF_AUDIO_MICROPHONE,
    ITF_HID_KEYBOARD,
    ITF_TOTAL,
};

enum {
    ENT_SPK_INPUT = 0x01,
    ENT_SPK_FEATURE = 0x02,
    ENT_SPK_OUTPUT = 0x03,
    ENT_CLOCK = 0x04,
    ENT_MIC_INPUT = 0x11,
    ENT_MIC_OUTPUT = 0x13,
};

#define VOLUME_CTRL_0_DB  0
#define VOLUME_CTRL_50_DB 12800
#define VOLUME_GAIN_Q15_0_DB 32767

#define UAC1_AUDIO_CONTROL_LEN 62
#define USB_AUDIO_DESC_LEN 183
#define USB_CONFIG_DESC_LEN (TUD_CONFIG_DESC_LEN + USB_AUDIO_DESC_LEN + TUD_HID_DESC_LEN)

static const tusb_desc_device_t s_device_descriptor = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = TUSB_CLASS_MISC,
    .bDeviceSubClass = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor = USB_VID_CAPPILOT,
    .idProduct = USB_PID_HEADSET,
    .bcdDevice = 0x0100,
    .iManufacturer = 0x01,
    .iProduct = 0x02,
    .iSerialNumber = 0x03,
    .bNumConfigurations = 0x01,
};

static const uint8_t s_hid_report_descriptor[] = {
    TUD_HID_REPORT_DESC_KEYBOARD(),
};

static const char *s_string_descriptor[] = {
    (char[]){0x09, 0x04},
    "CaPilot",
    "CaPilot USB Headset",
    "CaPilot-UAC1-001",
    "Microphone",
    "Speakers",
    "Keyboard",
};

static const uint8_t s_configuration_descriptor[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_TOTAL, 0, USB_CONFIG_DESC_LEN,
                          TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    8, TUSB_DESC_INTERFACE_ASSOCIATION, ITF_AUDIO_CONTROL, 3, TUSB_CLASS_AUDIO, AUDIO_SUBCLASS_CONTROL, 0, 0,
    9, TUSB_DESC_INTERFACE, ITF_AUDIO_CONTROL, 0, 0, TUSB_CLASS_AUDIO, AUDIO_SUBCLASS_CONTROL, 0, 0,
    10, TUSB_DESC_CS_INTERFACE, 0x01, U16_TO_U8S_LE(0x0100), U16_TO_U8S_LE(UAC1_AUDIO_CONTROL_LEN), 2,
    ITF_AUDIO_SPEAKER, ITF_AUDIO_MICROPHONE,
    12, TUSB_DESC_CS_INTERFACE, 0x02, ENT_SPK_INPUT, U16_TO_U8S_LE(AUDIO_TERM_TYPE_USB_STREAMING),
    0, 2, U16_TO_U8S_LE(0x0003), 0, 0,
    10, TUSB_DESC_CS_INTERFACE, 0x06, ENT_SPK_FEATURE, ENT_SPK_INPUT, 1, 0x03, 0x00, 0x00, 0,
    9, TUSB_DESC_CS_INTERFACE, 0x03, ENT_SPK_OUTPUT, U16_TO_U8S_LE(AUDIO_TERM_TYPE_OUT_HEADPHONES),
    0, ENT_SPK_FEATURE, 0,
    12, TUSB_DESC_CS_INTERFACE, 0x02, ENT_MIC_INPUT, U16_TO_U8S_LE(AUDIO_TERM_TYPE_IN_GENERIC_MIC),
    0, 1, U16_TO_U8S_LE(0x0001), 0, 0,
    9, TUSB_DESC_CS_INTERFACE, 0x03, ENT_MIC_OUTPUT, U16_TO_U8S_LE(AUDIO_TERM_TYPE_USB_STREAMING),
    0, ENT_MIC_INPUT, 0,
    9, TUSB_DESC_INTERFACE, ITF_AUDIO_SPEAKER, 0, 0, TUSB_CLASS_AUDIO, AUDIO_SUBCLASS_STREAMING, 0, 5,
    9, TUSB_DESC_INTERFACE, ITF_AUDIO_SPEAKER, 1, 1, TUSB_CLASS_AUDIO, AUDIO_SUBCLASS_STREAMING, 0, 5,
    7, TUSB_DESC_CS_INTERFACE, 0x01, ENT_SPK_INPUT, 1, U16_TO_U8S_LE(0x0001),
    11, TUSB_DESC_CS_INTERFACE, 0x02, 1, 2, 2, 16, 1, 0x80, 0xbb, 0x00,
    9, TUSB_DESC_ENDPOINT, USB_EP_AUDIO_OUT, TUSB_XFER_ISOCHRONOUS | TUSB_ISO_EP_ATT_ADAPTIVE,
    U16_TO_U8S_LE(192), 1, 0, 0,
    7, TUSB_DESC_CS_ENDPOINT, 0x01, 0x01, 0, U16_TO_U8S_LE(1),
    9, TUSB_DESC_INTERFACE, ITF_AUDIO_MICROPHONE, 0, 0, TUSB_CLASS_AUDIO, AUDIO_SUBCLASS_STREAMING, 0, 4,
    9, TUSB_DESC_INTERFACE, ITF_AUDIO_MICROPHONE, 1, 1, TUSB_CLASS_AUDIO, AUDIO_SUBCLASS_STREAMING, 0, 4,
    7, TUSB_DESC_CS_INTERFACE, 0x01, ENT_MIC_OUTPUT, 1, U16_TO_U8S_LE(0x0001),
    11, TUSB_DESC_CS_INTERFACE, 0x02, 1, 1, 2, 16, 1, 0x80, 0xbb, 0x00,
    9, TUSB_DESC_ENDPOINT, USB_EP_AUDIO_IN, TUSB_XFER_ISOCHRONOUS | TUSB_ISO_EP_ATT_ASYNCHRONOUS,
    U16_TO_U8S_LE(96), 1, 0, 0,
    7, TUSB_DESC_CS_ENDPOINT, 0x01, 0x01, 0, U16_TO_U8S_LE(0),
    TUD_HID_DESCRIPTOR(ITF_HID_KEYBOARD, 6, false, sizeof(s_hid_report_descriptor), USB_EP_HID_IN, 16, 10),
};

_Static_assert(sizeof(s_configuration_descriptor) == USB_CONFIG_DESC_LEN,
               "USB configuration descriptor length mismatch");
_Static_assert(CFG_TUD_AUDIO_FUNC_1_DESC_LEN == USB_AUDIO_DESC_LEN,
               "TinyUSB UAC2 descriptor length must match the emitted descriptor");

static bool s_initialized;
static uint32_t s_sample_rate = 48000;
static uint8_t s_mute[3];
static int16_t s_volume[3];
static volatile uint8_t s_speaker_alt;
static volatile uint8_t s_microphone_alt;

static uint8_t audio_channel_or_master(uint8_t channel)
{
    return channel < 3 ? channel : 0;
}

static uint16_t volume_db_q8_8_to_gain_q15(int16_t volume_db_q8_8)
{
    if (volume_db_q8_8 >= VOLUME_CTRL_0_DB) {
        return VOLUME_GAIN_Q15_0_DB;
    }
    if (volume_db_q8_8 <= -VOLUME_CTRL_50_DB) {
        return 0;
    }

    float db = (float)volume_db_q8_8 / 256.0f;
    float gain = powf(10.0f, db / 20.0f);
    int32_t q15 = (int32_t)(gain * (float)VOLUME_GAIN_Q15_0_DB + 0.5f);
    if (q15 < 0) {
        return 0;
    }
    if (q15 > VOLUME_GAIN_Q15_0_DB) {
        return VOLUME_GAIN_Q15_0_DB;
    }
    return (uint16_t)q15;
}

uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance)
{
    (void)instance;
    return s_hid_report_descriptor;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
                               hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
    (void)instance; (void)report_id; (void)report_type; (void)buffer; (void)reqlen;
    return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
                           hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize)
{
    (void)instance; (void)report_id; (void)report_type; (void)buffer; (void)bufsize;
}

static bool audio_clock_get(uint8_t rhport, const audio_control_request_t *request)
{
    if (request->bRequest == UAC1_REQ_GET_CUR) {
        uint8_t value[3] = { 0x80, 0xbb, 0x00 };
        return tud_audio_buffer_and_schedule_control_xfer(rhport,
            (const tusb_control_request_t *)request, value, sizeof(value));
    }
    if (request->bControlSelector == AUDIO_CS_CTRL_SAM_FREQ && request->bRequest == AUDIO_CS_REQ_CUR) {
        audio_control_cur_4_t value = { .bCur = (int32_t)tu_htole32(s_sample_rate) };
        return tud_audio_buffer_and_schedule_control_xfer(rhport,
            (const tusb_control_request_t *)request, &value, sizeof(value));
    }
    if (request->bControlSelector == AUDIO_CS_CTRL_SAM_FREQ && request->bRequest == AUDIO_CS_REQ_RANGE) {
        audio_control_range_4_n_t(1) value = { .wNumSubRanges = tu_htole16(1) };
        value.subrange[0].bMin = (int32_t)tu_htole32(48000);
        value.subrange[0].bMax = (int32_t)tu_htole32(48000);
        value.subrange[0].bRes = 0;
        return tud_audio_buffer_and_schedule_control_xfer(rhport,
            (const tusb_control_request_t *)request, &value, sizeof(value));
    }
    if (request->bControlSelector == AUDIO_CS_CTRL_CLK_VALID && request->bRequest == AUDIO_CS_REQ_CUR) {
        audio_control_cur_1_t value = { .bCur = 1 };
        return tud_audio_buffer_and_schedule_control_xfer(rhport,
            (const tusb_control_request_t *)request, &value, sizeof(value));
    }
    return false;
}

bool tud_audio_get_req_entity_cb(uint8_t rhport, tusb_control_request_t const *request)
{
    const audio_control_request_t *audio_request = (const audio_control_request_t *)request;
    if (audio_request->bEntityID == ENT_CLOCK) {
        return audio_clock_get(rhport, audio_request);
    }
    if (audio_request->bEntityID == ENT_SPK_FEATURE && audio_request->bControlSelector == AUDIO_FU_CTRL_MUTE
        && (audio_request->bRequest == AUDIO_CS_REQ_CUR || audio_request->bRequest == UAC1_REQ_GET_CUR)) {
        audio_control_cur_1_t value = { .bCur = s_mute[audio_channel_or_master(audio_request->bChannelNumber)] };
        return tud_audio_buffer_and_schedule_control_xfer(rhport, request, &value, sizeof(value));
    }
    if (audio_request->bEntityID == ENT_SPK_FEATURE && audio_request->bControlSelector == AUDIO_FU_CTRL_VOLUME) {
        if (audio_request->bRequest == UAC1_REQ_GET_MIN || audio_request->bRequest == UAC1_REQ_GET_MAX
            || audio_request->bRequest == UAC1_REQ_GET_RES) {
            int16_t raw = 256;
            if (audio_request->bRequest == UAC1_REQ_GET_MIN) {
                raw = -VOLUME_CTRL_50_DB;
            } else if (audio_request->bRequest == UAC1_REQ_GET_MAX) {
                raw = VOLUME_CTRL_0_DB;
            }
            audio_control_cur_2_t value = { .bCur = tu_htole16(raw) };
            return tud_audio_buffer_and_schedule_control_xfer(rhport, request, &value, sizeof(value));
        }
        if (audio_request->bRequest == AUDIO_CS_REQ_RANGE) {
            audio_control_range_2_n_t(1) value = {
                .wNumSubRanges = tu_htole16(1),
                .subrange[0] = {
                    .bMin = tu_htole16(-VOLUME_CTRL_50_DB),
                    .bMax = tu_htole16(VOLUME_CTRL_0_DB),
                    .bRes = tu_htole16(256),
                },
            };
            return tud_audio_buffer_and_schedule_control_xfer(rhport, request, &value, sizeof(value));
        }
        if (audio_request->bRequest == AUDIO_CS_REQ_CUR || audio_request->bRequest == UAC1_REQ_GET_CUR) {
            audio_control_cur_2_t value = {
                .bCur = tu_htole16(s_volume[audio_channel_or_master(audio_request->bChannelNumber)]),
            };
            return tud_audio_buffer_and_schedule_control_xfer(rhport, request, &value, sizeof(value));
        }
    }
    return false;
}

bool tud_audio_get_req_ep_cb(uint8_t rhport, tusb_control_request_t const *request)
{
    const uint8_t control_selector = TU_U16_HIGH(request->wValue);
    if (control_selector == UAC1_EP_CTRL_SAMPLING_FREQ && request->bRequest == UAC1_REQ_GET_CUR) {
        uint8_t value[3] = { 0x80, 0xbb, 0x00 };
        ESP_LOGI(TAG, "audio ep get sample rate: ep=0x%02x", TU_U16_LOW(request->wIndex));
        return tud_audio_buffer_and_schedule_control_xfer(rhport, request, value, sizeof(value));
    }
    return false;
}

bool tud_audio_set_itf_cb(uint8_t rhport, tusb_control_request_t const *request)
{
    (void)rhport;
    const uint8_t itf = TU_U16_LOW(request->wIndex);
    const uint8_t alt = TU_U16_LOW(request->wValue);
    ESP_LOGI(TAG, "audio set interface: itf=%u alt=%u", itf, alt);
    if (itf == ITF_AUDIO_SPEAKER) {
        s_speaker_alt = alt;
    } else if (itf == ITF_AUDIO_MICROPHONE) {
        s_microphone_alt = alt;
    }
    if (alt != 0) {
        tud_audio_clear_ep_out_ff();
        tud_audio_clear_ep_in_ff();
    }
    return true;
}

bool tud_audio_set_itf_close_ep_cb(uint8_t rhport, tusb_control_request_t const *request)
{
    (void)rhport;
    ESP_LOGI(TAG, "audio close interface: itf=%u alt=%u",
             TU_U16_LOW(request->wIndex), TU_U16_LOW(request->wValue));
    uint8_t const itf = TU_U16_LOW(request->wIndex);
    if (itf == ITF_AUDIO_SPEAKER) {
        s_speaker_alt = 0;
    } else if (itf == ITF_AUDIO_MICROPHONE) {
        s_microphone_alt = 0;
    }
    return true;
}

bool tud_audio_set_req_entity_cb(uint8_t rhport, tusb_control_request_t const *request, uint8_t *buffer)
{
    (void)rhport;
    const audio_control_request_t *audio_request = (const audio_control_request_t *)request;
    if (audio_request->bEntityID == ENT_CLOCK && audio_request->bControlSelector == AUDIO_CS_CTRL_SAM_FREQ
        && request->wLength == sizeof(audio_control_cur_4_t)) {
        const audio_control_cur_4_t *value = (const audio_control_cur_4_t *)buffer;
        return tu_le32toh(value->bCur) == 48000;
    }
    if (audio_request->bEntityID == ENT_SPK_FEATURE && audio_request->bChannelNumber < 3
        && audio_request->bControlSelector == AUDIO_FU_CTRL_MUTE && request->wLength == sizeof(audio_control_cur_1_t)) {
        uint8_t channel = audio_channel_or_master(audio_request->bChannelNumber);
        s_mute[channel] = ((audio_control_cur_1_t *)buffer)->bCur ? 1 : 0;
        ESP_LOGI(TAG, "speaker mute: channel=%u mute=%u", channel, s_mute[channel]);
        return true;
    }
    if (audio_request->bEntityID == ENT_SPK_FEATURE && audio_request->bChannelNumber < 3
        && audio_request->bControlSelector == AUDIO_FU_CTRL_VOLUME && request->wLength == sizeof(audio_control_cur_2_t)) {
        uint8_t channel = audio_channel_or_master(audio_request->bChannelNumber);
        int16_t volume = (int16_t)tu_le16toh(((audio_control_cur_2_t *)buffer)->bCur);
        if (volume > VOLUME_CTRL_0_DB) {
            volume = VOLUME_CTRL_0_DB;
        } else if (volume < -VOLUME_CTRL_50_DB) {
            volume = -VOLUME_CTRL_50_DB;
        }
        s_volume[channel] = volume;
        ESP_LOGI(TAG, "speaker volume: channel=%u db_q8_8=%d gain_q15=%u",
                 channel, volume, volume_db_q8_8_to_gain_q15(volume));
        return true;
    }
    return false;
}

bool tud_audio_set_req_ep_cb(uint8_t rhport, tusb_control_request_t const *request, uint8_t *buffer)
{
    (void)rhport;
    const uint8_t control_selector = TU_U16_HIGH(request->wValue);
    if (control_selector == UAC1_EP_CTRL_SAMPLING_FREQ && request->bRequest == UAC1_REQ_SET_CUR
        && request->wLength == 3) {
        const uint32_t rate = (uint32_t)buffer[0] | ((uint32_t)buffer[1] << 8) | ((uint32_t)buffer[2] << 16);
        ESP_LOGI(TAG, "audio ep set sample rate: ep=0x%02x rate=%" PRIu32,
                 TU_U16_LOW(request->wIndex), rate);
        return rate == s_sample_rate;
    }
    return false;
}

esp_err_t capilot_usb_headset_init(void)
{
    if (s_initialized) {
        return ESP_OK;
    }
    tinyusb_config_t config = TINYUSB_DEFAULT_CONFIG();
    config.descriptor.device = &s_device_descriptor;
    config.descriptor.full_speed_config = s_configuration_descriptor;
    config.descriptor.string = s_string_descriptor;
    config.descriptor.string_count = sizeof(s_string_descriptor) / sizeof(s_string_descriptor[0]);
    esp_err_t ret = tinyusb_driver_install(&config);
    if (ret == ESP_OK) {
        s_initialized = true;
        ESP_LOGI(TAG, "UAC1 headset + HID keyboard ready");
    }
    return ret;
}

bool capilot_usb_headset_is_connected(void)
{
    return tud_mounted();
}

uint16_t capilot_usb_headset_read_playback(void *buffer, uint16_t buffer_size)
{
    return tud_audio_read(buffer, buffer_size);
}

uint16_t capilot_usb_headset_write_capture(const void *buffer, uint16_t length)
{
    return tud_audio_write(buffer, length);
}

uint8_t capilot_usb_headset_get_speaker_alt(void)
{
    return s_speaker_alt;
}

uint8_t capilot_usb_headset_get_microphone_alt(void)
{
    return s_microphone_alt;
}

bool capilot_usb_headset_is_playback_muted(void)
{
    return s_mute[0] != 0;
}

int16_t capilot_usb_headset_get_playback_volume_db_q8_8(void)
{
    return s_volume[0];
}

uint16_t capilot_usb_headset_get_playback_gain_q15(void)
{
    if (capilot_usb_headset_is_playback_muted()) {
        return 0;
    }
    return volume_db_q8_8_to_gain_q15(s_volume[0]);
}

esp_err_t capilot_usb_headset_set_win_alt(bool pressed)
{
    if (!tud_mounted()) {
        ESP_LOGW(TAG, "Win+Alt ignored: USB is not mounted");
        return ESP_ERR_INVALID_STATE;
    }
    if (!tud_hid_ready()) {
        ESP_LOGW(TAG, "Win+Alt ignored: HID endpoint is not ready");
        return ESP_ERR_INVALID_STATE;
    }
    if (pressed) {
        tud_hid_keyboard_report(0, KEYBOARD_MODIFIER_LEFTGUI | KEYBOARD_MODIFIER_LEFTALT, NULL);
    } else {
        tud_hid_keyboard_report(0, 0, NULL);
    }
    return ESP_OK;
}
