#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Initialize the UAC1 headset and HID keyboard composite USB device. */
esp_err_t capilot_usb_headset_init(void);

/** Return true after the USB host has enumerated the device. */
bool capilot_usb_headset_is_connected(void);

/** Read PCM frames sent by the host to the headset speaker endpoint. */
uint16_t capilot_usb_headset_read_playback(void *buffer, uint16_t buffer_size);

/** Send PCM frames captured by the headset microphone to the host. */
uint16_t capilot_usb_headset_write_capture(const void *buffer, uint16_t length);

/** Current USB audio speaker alternate setting selected by the host. */
uint8_t capilot_usb_headset_get_speaker_alt(void);

/** Current USB audio microphone alternate setting selected by the host. */
uint8_t capilot_usb_headset_get_microphone_alt(void);

/** Return true when the host has muted the speaker feature unit. */
bool capilot_usb_headset_is_playback_muted(void);

/** Current speaker volume in USB Audio Q8.8 dB units, where 0 means 0 dB. */
int16_t capilot_usb_headset_get_playback_volume_db_q8_8(void);

/** Current speaker gain as Q1.15 linear amplitude, suitable for PCM scaling. */
uint16_t capilot_usb_headset_get_playback_gain_q15(void);

/** Hold or release the HID keyboard Win+Alt combination. */
esp_err_t capilot_usb_headset_set_win_alt(bool pressed);

#ifdef __cplusplus
}
#endif
