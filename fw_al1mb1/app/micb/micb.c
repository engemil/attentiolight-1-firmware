/*
MIT License

Copyright (c) 2026 EngEmil

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

/**
 * @file    micb.c
 * @brief   Multi-Interface Control Broker (MICB) implementation.
 *
 * @details Manages session state (STANDALONE/REMOTE), routes AP commands to
 *          handlers, enforces claim-based access control, and coordinates
 *          mode transitions with the animation engine and state machine.
 */

#include "micb.h"
#include "attentio_protocol.h"
#include "app_log.h"
#include "app_state_machine.h"
#include "animation_thread.h"
#include "persistent_data.h"
#include "app_header.h"
#include "modes.h"
#include "standalone_state.h"

#include "ch.h"
#include "hal.h"

#include <string.h>

/*===========================================================================*/
/* Constants                                                                 */
/*===========================================================================*/

/** @brief DFU magic RAM address (last 4 bytes of 24KB RAM). */
#define DFU_MAGIC_ADDR      ((volatile uint32_t *)0x20005FFCU)

/** @brief DFU magic value to trigger bootloader entry on reset. */
#define DFU_MAGIC_VALUE     0xDEADBEEFU

/** @brief Azure blue RGB for remote mode entry indicator. */
#define REMOTE_INDICATOR_R  0
#define REMOTE_INDICATOR_G  127
#define REMOTE_INDICATOR_B  255

/** @brief Default brightness for remote mode (0-255). */
#define REMOTE_DEFAULT_BRIGHTNESS   128

/* DEVICE_MODEL comes from board.h (e.g. "AttentioLight-1") */

/*===========================================================================*/
/* Module local variables                                                    */
/*===========================================================================*/

/** @brief Current session state. */
static micb_session_t micb_session;

/** @brief Send callbacks for each interface. */
static micb_send_fn_t micb_send_fns[MICB_IF_COUNT];

/** @brief Shared response buffer (used within micb_process_command only). */
static uint8_t resp_buf[AP_MAX_PACKET_SIZE];

/*===========================================================================*/
/* Interface name strings                                                    */
/*===========================================================================*/

static const char * const interface_names[] = {
    [MICB_IF_NONE]       = "NONE",
    [MICB_IF_STANDALONE] = "STANDALONE",
    [MICB_IF_USB]        = "USB",
    [MICB_IF_BLE]        = "BLE",
    [MICB_IF_WIFI]       = "WiFi",
};

static const char * const mode_names[] = {
    [MICB_MODE_STANDALONE] = "STANDALONE",
    [MICB_MODE_REMOTE]     = "REMOTE",
};

/*===========================================================================*/
/* Helper: send response to interface                                        */
/*===========================================================================*/

/**
 * @brief   Send a raw AP packet to an interface.
 */
static void micb_send_to(micb_interface_id_t iface,
                         const uint8_t *data, size_t len) {
    if (iface < MICB_IF_COUNT && micb_send_fns[iface] != NULL) {
        micb_send_fns[iface](data, len);
    }
}

/**
 * @brief   Send an OK response with optional payload to the requesting iface.
 */
static void micb_respond_ok(micb_interface_id_t iface,
                            const uint8_t *payload, uint8_t payload_len) {
    size_t n = ap_build_ok(resp_buf, sizeof(resp_buf), payload, payload_len);
    if (n > 0) {
        micb_send_to(iface, resp_buf, n);
    }
}

/**
 * @brief   Send an ERROR response to the requesting interface.
 */
static void micb_respond_error(micb_interface_id_t iface, ap_error_t err) {
    size_t n = ap_build_error(resp_buf, sizeof(resp_buf), err);
    if (n > 0) {
        micb_send_to(iface, resp_buf, n);
    }
}

/*===========================================================================*/
/* Mode transition helpers                                                   */
/*===========================================================================*/

/**
 * @brief   Transition from STANDALONE to REMOTE mode.
 */
static void micb_enter_remote(micb_interface_id_t iface) {
    LOG_INFO("MICB: STANDALONE -> REMOTE (controller=%s)",
             micb_interface_name(iface));

    micb_session.mode = MICB_MODE_REMOTE;
    micb_session.active_controller = iface;
    micb_session.session_counter++;
    if (micb_session.session_counter == 0) {
        micb_session.session_counter = 1;
    }
    micb_session.session_id = micb_session.session_counter;

    /* Tell the state machine we're entering external control. */
    app_sm_external_control_enter();

    /* Set azure blue indicator via animation engine. */
    anim_thread_set_solid(REMOTE_INDICATOR_R, REMOTE_INDICATOR_G,
                          REMOTE_INDICATOR_B, REMOTE_DEFAULT_BRIGHTNESS);
}

/**
 * @brief   Transition from REMOTE to STANDALONE mode.
 */
static void micb_exit_remote(void) {
    LOG_INFO("MICB: REMOTE -> STANDALONE (was controller=%s)",
             micb_interface_name(micb_session.active_controller));

    micb_session.mode = MICB_MODE_STANDALONE;
    micb_session.active_controller = MICB_IF_STANDALONE;
    micb_session.session_id = 0;

    /* Tell the state machine to resume standalone operation. */
    app_sm_external_control_exit();
}

/**
 * @brief   Send SESSION_END event to a specific interface.
 */
static void micb_send_session_end(micb_interface_id_t iface,
                                   ap_session_end_reason_t reason) {
    static uint8_t evt_buf[AP_MAX_PACKET_SIZE];
    uint8_t payload[3];
    payload[0] = (uint8_t)reason;
    payload[1] = (uint8_t)(micb_session.session_id >> 8);   /* session_id BE hi */
    payload[2] = (uint8_t)(micb_session.session_id & 0xFF); /* session_id BE lo */
    size_t n = ap_build_event(evt_buf, sizeof(evt_buf),
                              AP_CMD_EVT_SESSION_END, payload, sizeof(payload));
    if (n > 0) {
        micb_send_to(iface, evt_buf, n);
    }
}

/*===========================================================================*/
/* Claim checking                                                            */
/*===========================================================================*/

/**
 * @brief   Check if a command requires the sender to be the active controller.
 *
 * @param[in] cmd   AP command ID.
 * @return  true if the command requires a claim.
 */
static bool micb_cmd_requires_claim(uint8_t cmd) {
    switch (cmd) {
    /* LED control */
    case AP_CMD_LED_OFF:
    case AP_CMD_SET_RGB:
    case AP_CMD_SET_HSV:
    case AP_CMD_SET_BRIGHTNESS:
    case AP_CMD_SET_EFFECT:
    /* Power control */
    case AP_CMD_POWER_ON:
    case AP_CMD_POWER_OFF:
    /* Settings write */
    case AP_CMD_SETTINGS_SET:
        return true;
    default:
        return false;
    }
}

/*===========================================================================*/
/* Command Handlers                                                          */
/*===========================================================================*/

/* --- Session commands --- */

static void cmd_claim(micb_interface_id_t iface) {
    if (micb_session.mode == MICB_MODE_STANDALONE) {
        /* First claim — enter remote mode. */
        micb_enter_remote(iface);
    }
    else if (micb_session.active_controller == iface) {
        /* Already the controller — no-op. */
        LOG_DEBUG("MICB: CLAIM from %s (already controller)",
                  micb_interface_name(iface));
    }
    else {
        /* Takeover — notify old controller, switch to new. */
        LOG_INFO("MICB: CLAIM takeover %s -> %s",
                 micb_interface_name(micb_session.active_controller),
                 micb_interface_name(iface));
        micb_send_session_end(micb_session.active_controller,
                              AP_SESSION_END_TAKEOVER);
        micb_session.active_controller = iface;
        micb_session.session_counter++;
        if (micb_session.session_counter == 0) {
            micb_session.session_counter = 1;
        }
        micb_session.session_id = micb_session.session_counter;
    }

    /* Respond with 2-byte session ID (big-endian). */
    uint8_t sid_payload[2];
    sid_payload[0] = (uint8_t)(micb_session.session_id >> 8);
    sid_payload[1] = (uint8_t)(micb_session.session_id & 0xFF);
    micb_respond_ok(iface, sid_payload, sizeof(sid_payload));
}

static void cmd_release(micb_interface_id_t iface) {
    if (micb_session.mode == MICB_MODE_REMOTE &&
        micb_session.active_controller == iface) {
        micb_exit_remote();
    }
    else {
        LOG_DEBUG("MICB: RELEASE from %s (not controller, ignoring)",
                  micb_interface_name(iface));
    }

    micb_respond_ok(iface, NULL, 0);
}

static void cmd_ping(micb_interface_id_t iface) {
    micb_respond_ok(iface, NULL, 0);
}

/* --- Power commands --- */

static void cmd_power_on(micb_interface_id_t iface) {
    LOG_INFO("MICB: POWER_ON from %s", micb_interface_name(iface));
    /*
     * If the device is in OFF/low-power state, we need to wake it up.
     * Use the state machine's process_input to simulate a power-on event.
     * For now, send OK — the actual wake-up mechanism depends on the
     * system state.
     */
    app_sm_process_input(APP_SM_INPUT_BTN_SHORT);
    micb_respond_ok(iface, NULL, 0);
}

static void cmd_power_off(micb_interface_id_t iface) {
    LOG_INFO("MICB: POWER_OFF from %s", micb_interface_name(iface));

    /* Send SESSION_END to the controller before powering off. */
    if (micb_session.mode == MICB_MODE_REMOTE) {
        micb_send_session_end(micb_session.active_controller,
                              AP_SESSION_END_POWEROFF);
        micb_session.mode = MICB_MODE_STANDALONE;
        micb_session.active_controller = MICB_IF_STANDALONE;
        micb_session.session_id = 0;
        app_sm_external_control_exit();
    }

    /* Respond OK, then trigger power down. */
    micb_respond_ok(iface, NULL, 0);

    /* Simulate extended press start which triggers powerdown. */
    app_sm_process_input(APP_SM_INPUT_BTN_EXTENDED_START);
}

/* --- LED control commands --- */

static void cmd_led_off(micb_interface_id_t iface) {
    LOG_DEBUG("MICB: LED_OFF from %s", micb_interface_name(iface));
    anim_thread_set_solid(0, 0, 0, 0);
    micb_respond_ok(iface, NULL, 0);
}

static void cmd_set_rgb(micb_interface_id_t iface, const ap_packet_t *pkt) {
    if (pkt->payload_len != 3) {
        micb_respond_error(iface, AP_ERR_INVALID_PARAM);
        return;
    }

    uint8_t r = pkt->payload[0];
    uint8_t g = pkt->payload[1];
    uint8_t b = pkt->payload[2];

    LOG_DEBUG("MICB: SET_RGB(%u,%u,%u) from %s", r, g, b,
              micb_interface_name(iface));

    anim_thread_set_solid(r, g, b, 255);
    micb_respond_ok(iface, NULL, 0);
}

static void cmd_set_hsv(micb_interface_id_t iface, const ap_packet_t *pkt) {
    if (pkt->payload_len != 4) {
        micb_respond_error(iface, AP_ERR_INVALID_PARAM);
        return;
    }

    /* H is 2 bytes (little-endian), S and V are 1 byte each. */
    uint16_t h = (uint16_t)pkt->payload[0] | ((uint16_t)pkt->payload[1] << 8);
    uint8_t  s = pkt->payload[2];
    uint8_t  v = pkt->payload[3];

    LOG_DEBUG("MICB: SET_HSV(H=%u,S=%u,V=%u) from %s", h, s, v,
              micb_interface_name(iface));

    if (h > 359 || s > 100 || v > 100) {
        micb_respond_error(iface, AP_ERR_INVALID_PARAM);
        return;
    }

    /*
     * Convert HSV to RGB.
     * H: 0-359, S: 0-100, V: 0-100
     */
    uint8_t r, g, b;
    if (s == 0) {
        r = g = b = (uint8_t)((v * 255) / 100);
    }
    else {
        uint16_t region = h / 60;
        uint16_t remainder = (h - (region * 60)) * 6;

        uint8_t p = (uint8_t)((v * (100 - s) * 255) / 10000);
        uint8_t q = (uint8_t)((v * (10000 - s * remainder) * 255) / 1000000);
        uint8_t t = (uint8_t)((v * (10000 - s * (360 - remainder)) * 255) / 1000000);
        uint8_t val = (uint8_t)((v * 255) / 100);

        switch (region) {
        case 0:  r = val; g = t;   b = p;   break;
        case 1:  r = q;   g = val; b = p;   break;
        case 2:  r = p;   g = val; b = t;   break;
        case 3:  r = p;   g = q;   b = val; break;
        case 4:  r = t;   g = p;   b = val; break;
        default: r = val; g = p;   b = q;   break;
        }
    }

    anim_thread_set_solid(r, g, b, 255);
    micb_respond_ok(iface, NULL, 0);
}

static void cmd_set_brightness(micb_interface_id_t iface,
                               const ap_packet_t *pkt) {
    if (pkt->payload_len != 1) {
        micb_respond_error(iface, AP_ERR_INVALID_PARAM);
        return;
    }

    uint8_t brightness_pct = pkt->payload[0];
    if (brightness_pct > 100) {
        micb_respond_error(iface, AP_ERR_INVALID_PARAM);
        return;
    }

    LOG_DEBUG("MICB: SET_BRIGHTNESS(%u%%) from %s", brightness_pct,
              micb_interface_name(iface));

    /* Convert percentage (0-100) to 0-255 and re-apply the current color. */
    uint8_t brightness_255 = (uint8_t)((brightness_pct * 255) / 100);
    uint8_t r, g, b;
    anim_thread_get_target_rgb(&r, &g, &b);
    anim_thread_set_solid(r, g, b, brightness_255);
    micb_respond_ok(iface, NULL, 0);
}

/* --- Query commands --- */

static void cmd_get_status(micb_interface_id_t iface) {
    uint8_t payload[14];
    uint8_t r, g, b;

    anim_thread_get_target_rgb(&r, &g, &b);

    payload[0]  = (uint8_t)app_sm_get_system_state();       /* system_state      */
    payload[1]  = r;                                        /* current_r         */
    payload[2]  = g;                                        /* current_g         */
    payload[3]  = b;                                        /* current_b         */
    payload[4]  = (uint8_t)((anim_thread_get_brightness() * 100U) / 255U);
                                                            /* brightness 0-100% */
    payload[5]  = (uint8_t)micb_session.mode;               /* control_mode      */
    payload[6]  = (uint8_t)micb_session.active_controller;  /* active_controller */
    payload[7]  = (uint8_t)app_sm_get_mode();               /* standalone_mode   */
    payload[8]  = (uint8_t)mode_effects_get_submode();      /* effects_submode   */
    payload[9]  = standalone_color_index;                    /* color_index 0-11  */
    payload[10] = standalone_brightness;                     /* standalone brightness 0-255 */
    payload[11] = (uint8_t)anim_thread_get_current_type();  /* anim_type         */
    payload[12] = (uint8_t)(micb_session.session_id >> 8);  /* session_id BE hi  */
    payload[13] = (uint8_t)(micb_session.session_id & 0xFF);/* session_id BE lo  */

    micb_respond_ok(iface, payload, sizeof(payload));
}

/*===========================================================================*/
/* Metadata helpers                                                          */
/*===========================================================================*/

/**
 * @brief   STM32C0xx unique device ID register base address.
 *          96-bit UID stored as 3 x 32-bit words.
 */
#define MICB_STM32_UID_BASE     0x1FFF7550U

/**
 * @brief   Single metadata key-value entry (string pointers, not owned).
 */
typedef struct {
    const char *key;
    const char *val;
    uint8_t     val_len;
} md_entry_t;

/**
 * @brief   Format a uint8 as a 1-3 digit decimal string.
 * @return  Number of characters written (no NUL terminator).
 */
__attribute__((noinline))
static uint8_t u8_to_dec(char *buf, uint8_t v) {
    uint8_t n = 0;
    if (v >= 100) { buf[n++] = '0' + (v / 100); v %= 100; }
    if (n > 0 || v >= 10) { buf[n++] = '0' + (v / 10); v %= 10; }
    buf[n++] = '0' + v;
    return n;
}

/**
 * @brief   Format a uint32 as a decimal string.
 * @return  Number of characters written (no NUL terminator).
 */
__attribute__((noinline))
static uint8_t u32_to_dec(char *buf, uint32_t v) {
    char tmp[10];   /* max 10 digits for uint32 */
    uint8_t n = 0;

    if (v == 0) {
        buf[0] = '0';
        return 1;
    }
    while (v > 0) {
        tmp[n++] = '0' + (char)(v % 10);
        v /= 10;
    }
    /* Reverse into output buffer. */
    for (uint8_t i = 0; i < n; i++) {
        buf[i] = tmp[n - 1 - i];
    }
    return n;
}

/**
 * @brief   Build the full metadata table.
 *
 * @param[out] entries  Array to fill (must be at least METADATA_MAX_ENTRIES).
 * @param[out] scratch  Scratch buffer for formatted strings.
 * @param[in]  scratch_sz Size of scratch buffer.
 * @return  Number of entries written.
 *
 * @note    Entries point into static strings, board.h defines, and the
 *          scratch buffer.  Caller must consume entries before scratch is
 *          reused.
 */
#define METADATA_MAX_ENTRIES    16

__attribute__((noinline))
static uint8_t metadata_build_table(md_entry_t *entries,
                                    char *scratch, size_t scratch_sz) {
    uint8_t n = 0;
    size_t  spos = 0;   /* current position in scratch */

    /* Helper: reserve space in scratch, return pointer. */
    #define SCRATCH_ALLOC(need) (                                       \
        ((spos + (need)) <= scratch_sz) ? &scratch[(spos += (need)) - (need)] : NULL \
    )

    /* --- Device identity --- */

    {
        const pd_data_t *pd = persistent_data_get();
        const char *name = (pd != NULL) ? pd->device_name : "AttentioLight";
        entries[n].key = "device_name";
        entries[n].val = name;
        entries[n].val_len = (uint8_t)strlen(name);
        n++;
    }

    entries[n].key = "device_model";
    entries[n].val = DEVICE_MODEL;
    entries[n].val_len = (uint8_t)strlen(DEVICE_MODEL);
    n++;

    entries[n].key = "hardware_revision";
    entries[n].val = HARDWARE_REVISION;
    entries[n].val_len = (uint8_t)strlen(HARDWARE_REVISION);
    n++;

    /* Chip UID as 24 uppercase hex characters. */
    {
        static const char hex_chars[] = "0123456789ABCDEF";
        const uint32_t *uid = (const uint32_t *)MICB_STM32_UID_BASE;
        char *uid_str = SCRATCH_ALLOC(24);
        if (uid_str != NULL) {
            for (unsigned i = 0; i < 3; i++) {
                uint32_t w = uid[i];
                uid_str[i * 8 + 0] = hex_chars[(w >> 28) & 0xFU];
                uid_str[i * 8 + 1] = hex_chars[(w >> 24) & 0xFU];
                uid_str[i * 8 + 2] = hex_chars[(w >> 20) & 0xFU];
                uid_str[i * 8 + 3] = hex_chars[(w >> 16) & 0xFU];
                uid_str[i * 8 + 4] = hex_chars[(w >> 12) & 0xFU];
                uid_str[i * 8 + 5] = hex_chars[(w >>  8) & 0xFU];
                uid_str[i * 8 + 6] = hex_chars[(w >>  4) & 0xFU];
                uid_str[i * 8 + 7] = hex_chars[(w >>  0) & 0xFU];
            }

            entries[n].key = "serial_number";
            entries[n].val = uid_str;
            entries[n].val_len = 24;
            n++;

            entries[n].key = "chip_uid";
            entries[n].val = uid_str;   /* same value */
            entries[n].val_len = 24;
            n++;
        }
    }

    /* --- Firmware build --- */

    /* Firmware version "M.m.p". */
    {
        uint8_t fw_major = (uint8_t)((app_header.version >> 16) & 0xFF);
        uint8_t fw_minor = (uint8_t)((app_header.version >> 8)  & 0xFF);
        uint8_t fw_patch = (uint8_t)((app_header.version >> 0)  & 0xFF);

        char *vbuf = SCRATCH_ALLOC(12);
        if (vbuf != NULL) {
            uint8_t vlen = 0;
            vlen += u8_to_dec(&vbuf[vlen], fw_major);
            vbuf[vlen++] = '.';
            vlen += u8_to_dec(&vbuf[vlen], fw_minor);
            vbuf[vlen++] = '.';
            vlen += u8_to_dec(&vbuf[vlen], fw_patch);

            entries[n].key = "firmware_version";
            entries[n].val = vbuf;
            entries[n].val_len = vlen;
            n++;
        }
    }

    entries[n].key = "build_date";
    entries[n].val = __DATE__;
    entries[n].val_len = (uint8_t)strlen(__DATE__);
    n++;

    entries[n].key = "build_time";
    entries[n].val = __TIME__;
    entries[n].val_len = (uint8_t)strlen(__TIME__);
    n++;

    entries[n].key = "compiler";
    entries[n].val = __VERSION__;
    entries[n].val_len = (uint8_t)strlen(__VERSION__);
    n++;

    /* Protocol version as decimal string. */
    {
        char *pvbuf = SCRATCH_ALLOC(4);
        if (pvbuf != NULL) {
            uint8_t pvlen = u8_to_dec(pvbuf, AP_PROTOCOL_VERSION);
            entries[n].key = "protocol_version";
            entries[n].val = pvbuf;
            entries[n].val_len = pvlen;
            n++;
        }
    }

    /* --- Platform --- */

    entries[n].key = "platform";
    entries[n].val = "STM32C071";
    entries[n].val_len = 9;
    n++;

    entries[n].key = "architecture";
    entries[n].val = "ARMv6-M";
    entries[n].val_len = 7;
    n++;

    entries[n].key = "core_variant";
    entries[n].val = "Cortex-M0+";
    entries[n].val_len = 10;
    n++;

    {
        const char *ker = CH_KERNEL_VERSION;
        entries[n].key = "chibios_kernel";
        entries[n].val = ker;
        entries[n].val_len = (uint8_t)strlen(ker);
        n++;
    }

    {
        const char *pi = PORT_INFO;
        entries[n].key = "chibios_port_info";
        entries[n].val = pi;
        entries[n].val_len = (uint8_t)strlen(pi);
        n++;
    }

    /* --- Runtime --- */

    /* Uptime in seconds. */
    {
        systime_t now = chVTGetSystemTimeX();
        uint32_t secs = (uint32_t)(now / (systime_t)CH_CFG_ST_FREQUENCY);
        char *ubuf = SCRATCH_ALLOC(10);
        if (ubuf != NULL) {
            uint8_t ulen = u32_to_dec(ubuf, secs);
            entries[n].key = "uptime";
            entries[n].val = ubuf;
            entries[n].val_len = ulen;
            n++;
        }
    }

    #undef SCRATCH_ALLOC
    return n;
}

/**
 * @brief   Find a metadata entry by key name.
 *
 * @param[in]  entries      Metadata table.
 * @param[in]  count        Number of entries.
 * @param[in]  key          Key string to match (not NUL-terminated).
 * @param[in]  key_len      Length of key.
 * @return     Pointer to matching entry, or NULL if not found.
 */
__attribute__((noinline))
static const md_entry_t *metadata_find(const md_entry_t *entries,
                                       uint8_t count,
                                       const char *key, uint8_t key_len) {
    for (uint8_t i = 0; i < count; i++) {
        if (strlen(entries[i].key) == key_len &&
            memcmp(entries[i].key, key, key_len) == 0) {
            return &entries[i];
        }
    }
    return NULL;
}

/**
 * @brief   Shared response payload for metadata command handlers.
 * @note    Safe because micb_process_command is single-threaded dispatch.
 */
static uint8_t md_payload[AP_MAX_PAYLOAD_SIZE];

/**
 * @brief   Handle GET_METADATA (0x43) — paginated list of all metadata.
 *
 * @details Request payload: empty or [page:1] (0-based page number).
 *          Response payload:
 *            [total_count:1][page:1][page_count:1]
 *            { [key_len:1][key:N][val_len:1][val:M] } * page_count
 *
 *          The firmware fills as many KV pairs as fit within
 *          AP_MAX_PAYLOAD_SIZE for the requested page.  The client
 *          requests successive pages until all entries are received.
 */
__attribute__((noinline))
static void cmd_get_metadata(micb_interface_id_t iface,
                              const ap_packet_t *pkt) {
    /* Build the full metadata table. */
    md_entry_t entries[METADATA_MAX_ENTRIES];
    char scratch[128];
    uint8_t total = metadata_build_table(entries, scratch, sizeof(scratch));

    /* Determine requested page. */
    uint8_t req_page = 0;
    if (pkt->payload_len >= 1) {
        req_page = pkt->payload[0];
    }

    /*
     * Walk through entries, skipping those belonging to earlier pages,
     * to find the starting index for the requested page.
     */
    const size_t header_sz = 3;     /* total_count + page + page_count */
    size_t idx = header_sz;
    uint8_t page_count = 0;
    uint8_t current_page = 0;
    uint8_t entry_i = 0;

    /* Simulate earlier pages to find the start of req_page. */
    while (current_page < req_page && entry_i < total) {
        size_t sim_idx = header_sz;
        while (entry_i < total) {
            uint8_t kl = (uint8_t)strlen(entries[entry_i].key);
            uint8_t vl = entries[entry_i].val_len;
            size_t need = 1 + kl + 1 + vl;
            if ((sim_idx + need) > AP_MAX_PAYLOAD_SIZE) {
                break;  /* this entry starts a new page */
            }
            sim_idx += need;
            entry_i++;
        }
        current_page++;
    }

    if (current_page != req_page) {
        /* Requested page is beyond available data. */
        micb_respond_error(iface, AP_ERR_INVALID_PARAM);
        return;
    }

    /* Fill payload with entries for this page. */
    while (entry_i < total) {
        uint8_t kl = (uint8_t)strlen(entries[entry_i].key);
        uint8_t vl = entries[entry_i].val_len;
        size_t need = 1 + kl + 1 + vl;
        if ((idx + need) > AP_MAX_PAYLOAD_SIZE) {
            break;  /* no more room on this page */
        }
        md_payload[idx++] = kl;
        memcpy(&md_payload[idx], entries[entry_i].key, kl);
        idx += kl;
        md_payload[idx++] = vl;
        memcpy(&md_payload[idx], entries[entry_i].val, vl);
        idx += vl;
        page_count++;
        entry_i++;
    }

    /* Fill in the 3-byte header. */
    md_payload[0] = total;
    md_payload[1] = req_page;
    md_payload[2] = page_count;

    micb_respond_ok(iface, md_payload, (uint8_t)idx);
}

/**
 * @brief   Handle METADATA_GET (0x44) — get a single metadata field by key.
 *
 * @details Request payload: [key_len:1][key:N]
 *          Response payload: [key_len:1][key:N][val_len:1][val:M]
 *          Same single-pair format as SETTINGS_GET (0x51).
 */
__attribute__((noinline))
static void cmd_metadata_get(micb_interface_id_t iface,
                              const ap_packet_t *pkt) {
    if (pkt->payload_len < 2) {
        micb_respond_error(iface, AP_ERR_INVALID_PARAM);
        return;
    }

    uint8_t key_len = pkt->payload[0];
    if (key_len == 0 || key_len > pkt->payload_len - 1) {
        micb_respond_error(iface, AP_ERR_INVALID_PARAM);
        return;
    }

    const char *key = (const char *)&pkt->payload[1];

    /* Build full table, then look up the requested key. */
    md_entry_t entries[METADATA_MAX_ENTRIES];
    char scratch[128];
    uint8_t total = metadata_build_table(entries, scratch, sizeof(scratch));

    const md_entry_t *found = metadata_find(entries, total, key, key_len);
    if (found == NULL) {
        micb_respond_error(iface, AP_ERR_INVALID_PARAM);
        return;
    }

    /* Build response: [key_len][key][val_len][val] */
    size_t idx = 0;

    md_payload[idx++] = key_len;
    memcpy(&md_payload[idx], key, key_len);
    idx += key_len;
    md_payload[idx++] = found->val_len;
    memcpy(&md_payload[idx], found->val, found->val_len);
    idx += found->val_len;

    micb_respond_ok(iface, md_payload, (uint8_t)idx);
}

/* --- Settings commands --- */

static void cmd_settings_list(micb_interface_id_t iface) {
    const pd_data_t *pd = persistent_data_get();
    if (pd == NULL) {
        micb_respond_error(iface, AP_ERR_INVALID_STATE);
        return;
    }

    static uint8_t payload[AP_MAX_PAYLOAD_SIZE];
    size_t idx = 1;     /* byte 0 = count */
    uint8_t count = 0;

    /* device_name setting. */
    {
        const char *key = "device_name";
        uint8_t kl = (uint8_t)strlen(key);
        uint8_t vl = (uint8_t)strlen(pd->device_name);
        if ((idx + 1 + kl + 1 + vl) <= AP_MAX_PAYLOAD_SIZE) {
            payload[idx++] = kl;
            memcpy(&payload[idx], key, kl); idx += kl;
            payload[idx++] = vl;
            memcpy(&payload[idx], pd->device_name, vl); idx += vl;
            count++;
        }
    }

    /* log_level setting. */
    {
        const char *key = "default_loglevel";
        uint8_t kl = (uint8_t)strlen(key);
        char val_str[4];
        uint8_t lv = pd->log_level;
        uint8_t vl;
        if (lv >= 10) {
            val_str[0] = '0' + (lv / 10);
            val_str[1] = '0' + (lv % 10);
            vl = 2;
        } else {
            val_str[0] = '0' + lv;
            vl = 1;
        }
        if ((idx + 1 + kl + 1 + vl) <= AP_MAX_PAYLOAD_SIZE) {
            payload[idx++] = kl;
            memcpy(&payload[idx], key, kl); idx += kl;
            payload[idx++] = vl;
            memcpy(&payload[idx], val_str, vl); idx += vl;
            count++;
        }
    }

    payload[0] = count;
    micb_respond_ok(iface, payload, (uint8_t)idx);
}

static void cmd_settings_get(micb_interface_id_t iface,
                             const ap_packet_t *pkt) {
    if (pkt->payload_len < 2) {
        micb_respond_error(iface, AP_ERR_INVALID_PARAM);
        return;
    }

    uint8_t key_len = pkt->payload[0];
    if (key_len == 0 || key_len > pkt->payload_len - 1) {
        micb_respond_error(iface, AP_ERR_INVALID_PARAM);
        return;
    }

    const char *key = (const char *)&pkt->payload[1];
    const pd_data_t *pd = persistent_data_get();
    if (pd == NULL) {
        micb_respond_error(iface, AP_ERR_INVALID_STATE);
        return;
    }

    static uint8_t payload[AP_MAX_PAYLOAD_SIZE];
    size_t idx = 0;

    /* Echo key back. */
    payload[idx++] = key_len;
    memcpy(&payload[idx], key, key_len);
    idx += key_len;

    if (key_len == 11 && memcmp(key, "device_name", 11) == 0) {
        uint8_t vl = (uint8_t)strlen(pd->device_name);
        payload[idx++] = vl;
        memcpy(&payload[idx], pd->device_name, vl);
        idx += vl;
    }
    else if (key_len == 16 && memcmp(key, "default_loglevel", 16) == 0) {
        char val_str[4];
        uint8_t lv = pd->log_level;
        uint8_t vl;
        if (lv >= 10) {
            val_str[0] = '0' + (lv / 10);
            val_str[1] = '0' + (lv % 10);
            vl = 2;
        } else {
            val_str[0] = '0' + lv;
            vl = 1;
        }
        payload[idx++] = vl;
        memcpy(&payload[idx], val_str, vl);
        idx += vl;
    }
    else {
        /* Unknown key. */
        micb_respond_error(iface, AP_ERR_INVALID_PARAM);
        return;
    }

    micb_respond_ok(iface, payload, (uint8_t)idx);
}

static void cmd_settings_set(micb_interface_id_t iface,
                             const ap_packet_t *pkt) {
    /* Format: [key_len:1][key:N][val_len:1][val:M] */
    if (pkt->payload_len < 3) {
        micb_respond_error(iface, AP_ERR_INVALID_PARAM);
        return;
    }

    uint8_t key_len = pkt->payload[0];
    if (key_len == 0 || (1 + key_len + 1) > pkt->payload_len) {
        micb_respond_error(iface, AP_ERR_INVALID_PARAM);
        return;
    }

    const char *key = (const char *)&pkt->payload[1];
    uint8_t val_len = pkt->payload[1 + key_len];
    if ((1 + key_len + 1 + val_len) > pkt->payload_len) {
        micb_respond_error(iface, AP_ERR_INVALID_PARAM);
        return;
    }
    const char *val = (const char *)&pkt->payload[1 + key_len + 1];

    pd_result_t result;

    if (key_len == 11 && memcmp(key, "device_name", 11) == 0) {
        /* Need null-terminated copy. */
        char name_buf[PD_DEVICE_NAME_SIZE];
        size_t copy_len = (val_len < PD_DEVICE_NAME_SIZE - 1)
                          ? val_len : (PD_DEVICE_NAME_SIZE - 1);
        memcpy(name_buf, val, copy_len);
        name_buf[copy_len] = '\0';

        result = persistent_data_set_device_name(name_buf);
        if (result != PD_OK) {
            micb_respond_error(iface, AP_ERR_INVALID_PARAM);
            return;
        }
        result = persistent_data_save();
        if (result != PD_OK) {
            LOG_ERROR("MICB: Failed to save device_name: %s",
                      persistent_data_result_str(result));
        }
    }
    else if (key_len == 16 && memcmp(key, "default_loglevel", 16) == 0) {
        /* Parse numeric value. */
        if (val_len != 1 || val[0] < '0' || val[0] > '4') {
            micb_respond_error(iface, AP_ERR_INVALID_PARAM);
            return;
        }
        uint8_t level = (uint8_t)(val[0] - '0');
        result = persistent_data_set_log_level(level);
        if (result != PD_OK) {
            micb_respond_error(iface, AP_ERR_INVALID_PARAM);
            return;
        }
        result = persistent_data_save();
        if (result != PD_OK) {
            LOG_ERROR("MICB: Failed to save default_loglevel: %s",
                      persistent_data_result_str(result));
        }
        /* Also update runtime log level. */
        log_set_level(level);
    }
    else {
        micb_respond_error(iface, AP_ERR_INVALID_PARAM);
        return;
    }

    micb_respond_ok(iface, NULL, 0);
}

/* --- Log control commands --- */

static void cmd_log_get_level(micb_interface_id_t iface) {
    uint8_t level = log_get_level();
    micb_respond_ok(iface, &level, 1);
}

static void cmd_log_set_level(micb_interface_id_t iface,
                              const ap_packet_t *pkt) {
    if (pkt->payload_len != 1) {
        micb_respond_error(iface, AP_ERR_INVALID_PARAM);
        return;
    }

    uint8_t level = pkt->payload[0];
    if (level > 4) {
        micb_respond_error(iface, AP_ERR_INVALID_PARAM);
        return;
    }

    LOG_SYS("MICB: LOG_SET_LEVEL(%u) from %s", level,
            micb_interface_name(iface));

    log_set_level(level);
    micb_respond_ok(iface, NULL, 0);
}

/* --- DFU command --- */

static void cmd_dfu_enter(micb_interface_id_t iface) {
    LOG_SYS("MICB: DFU_ENTER from %s — rebooting to bootloader",
             micb_interface_name(iface));

    /* Send OK first so the host knows we accepted the command. */
    micb_respond_ok(iface, NULL, 0);

    /* Small delay to allow the response to be sent over USB. */
    chThdSleepMilliseconds(50);

    /* Write DFU magic to RAM and reset. */
    *DFU_MAGIC_ADDR = DFU_MAGIC_VALUE;
    NVIC_SystemReset();

    /* Should never reach here. */
    while (1) {}
}

/*===========================================================================*/
/* Public API                                                                */
/*===========================================================================*/

void micb_init(void) {
    micb_session.mode = MICB_MODE_STANDALONE;
    micb_session.active_controller = MICB_IF_STANDALONE;
    micb_session.session_id = 0;
    micb_session.session_counter = 0;

    memset(micb_send_fns, 0, sizeof(micb_send_fns));

    LOG_SYS("MICB: Initialized (mode=STANDALONE)");
}

void micb_register_interface(micb_interface_id_t iface,
                             micb_send_fn_t send_fn) {
    if (iface < MICB_IF_COUNT) {
        micb_send_fns[iface] = send_fn;
        LOG_SYS("MICB: Registered interface %s", micb_interface_name(iface));
    }
}

void micb_process_command(micb_interface_id_t iface, const ap_packet_t *pkt) {
    /* Check claim requirement. */
    if (micb_cmd_requires_claim(pkt->cmd)) {
        if (micb_session.mode != MICB_MODE_REMOTE ||
            micb_session.active_controller != iface) {
            LOG_WARN("MICB: CMD 0x%02X from %s rejected (not controller)",
                     pkt->cmd, micb_interface_name(iface));
            micb_respond_error(iface, AP_ERR_NOT_CONTROLLER);
            return;
        }
    }

    /* Dispatch to command handler. */
    switch (pkt->cmd) {

    /* Session */
    case AP_CMD_CLAIM:          cmd_claim(iface);               break;
    case AP_CMD_RELEASE:        cmd_release(iface);             break;
    case AP_CMD_PING:           cmd_ping(iface);                break;

    /* Power */
    case AP_CMD_POWER_ON:       cmd_power_on(iface);            break;
    case AP_CMD_POWER_OFF:      cmd_power_off(iface);           break;

    /* LED control */
    case AP_CMD_LED_OFF:        cmd_led_off(iface);             break;
    case AP_CMD_SET_RGB:        cmd_set_rgb(iface, pkt);        break;
    case AP_CMD_SET_HSV:        cmd_set_hsv(iface, pkt);        break;
    case AP_CMD_SET_BRIGHTNESS: cmd_set_brightness(iface, pkt); break;

    /* Query */
    case AP_CMD_GET_STATUS:     cmd_get_status(iface);          break;
    case AP_CMD_GET_METADATA:   cmd_get_metadata(iface, pkt);   break;
    case AP_CMD_METADATA_GET:   cmd_metadata_get(iface, pkt);   break;

    /* Settings */
    case AP_CMD_SETTINGS_LIST:  cmd_settings_list(iface);       break;
    case AP_CMD_SETTINGS_GET:   cmd_settings_get(iface, pkt);   break;
    case AP_CMD_SETTINGS_SET:   cmd_settings_set(iface, pkt);   break;

    /* Log control */
    case AP_CMD_LOG_GET_LEVEL:  cmd_log_get_level(iface);       break;
    case AP_CMD_LOG_SET_LEVEL:  cmd_log_set_level(iface, pkt);  break;

    /* DFU */
    case AP_CMD_DFU_ENTER:      cmd_dfu_enter(iface);           break;

    /* Unknown command */
    default:
        LOG_WARN("MICB: Unknown CMD 0x%02X from %s",
                 pkt->cmd, micb_interface_name(iface));
        micb_respond_error(iface, AP_ERR_INVALID_CMD);
        break;
    }
}

void micb_forward_button_event(ap_button_event_t event) {
    if (micb_session.mode != MICB_MODE_REMOTE) {
        return;
    }

    static uint8_t evt_buf[AP_MAX_PACKET_SIZE];
    uint8_t payload = (uint8_t)event;
    size_t n = ap_build_event(evt_buf, sizeof(evt_buf),
                              AP_CMD_EVT_BUTTON, &payload, 1);
    if (n > 0) {
        micb_send_to(micb_session.active_controller, evt_buf, n);
    }
}

const micb_session_t *micb_get_session(void) {
    return &micb_session;
}

micb_control_mode_t micb_get_mode(void) {
    return micb_session.mode;
}

micb_interface_id_t micb_get_active_controller(void) {
    return micb_session.active_controller;
}

bool micb_is_controller(micb_interface_id_t iface) {
    return (micb_session.mode == MICB_MODE_REMOTE &&
            micb_session.active_controller == iface);
}

const char *micb_interface_name(micb_interface_id_t iface) {
    if (iface < MICB_IF_COUNT) {
        return interface_names[iface];
    }
    return "UNKNOWN";
}

const char *micb_mode_name(micb_control_mode_t mode) {
    if (mode <= MICB_MODE_REMOTE) {
        return mode_names[mode];
    }
    return "UNKNOWN";
}
