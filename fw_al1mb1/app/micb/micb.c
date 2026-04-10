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

    /* Tell the state machine to resume standalone operation. */
    app_sm_external_control_exit();
}

/**
 * @brief   Send SESSION_END event to a specific interface.
 */
static void micb_send_session_end(micb_interface_id_t iface,
                                   ap_session_end_reason_t reason) {
    static uint8_t evt_buf[AP_MAX_PACKET_SIZE];
    uint8_t payload = (uint8_t)reason;
    size_t n = ap_build_event(evt_buf, sizeof(evt_buf),
                              AP_CMD_EVT_SESSION_END, &payload, 1);
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
    }

    micb_respond_ok(iface, NULL, 0);
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

    /*
     * Convert percentage (0-100) to 0-255 range.
     * This sets a solid color at the current RGB with new brightness.
     * For now, we send a solid white at the given brightness as a
     * brightness-only control. Future: track current remote RGB.
     */
    uint8_t brightness_255 = (uint8_t)((brightness_pct * 255) / 100);
    anim_thread_set_solid(255, 255, 255, brightness_255);
    micb_respond_ok(iface, NULL, 0);
}

/* --- Query commands --- */

static void cmd_get_state(micb_interface_id_t iface) {
    uint8_t payload[8];

    payload[0] = (uint8_t)app_sm_get_system_state();    /* system_state */
    /* Current RGB — not easily accessible from animation state.
     * Send zeros for now. Future: expose current animation RGB. */
    payload[1] = 0;    /* current_r */
    payload[2] = 0;    /* current_g */
    payload[3] = 0;    /* current_b */
    payload[4] = 0;    /* brightness (0-100) */
    payload[5] = (uint8_t)micb_session.mode;             /* control_mode */
    payload[6] = (uint8_t)micb_session.active_controller;/* active_controller */
    payload[7] = (uint8_t)app_sm_get_mode();             /* standalone_mode */

    micb_respond_ok(iface, payload, sizeof(payload));
}

static void cmd_get_caps(micb_interface_id_t iface) {
    uint8_t payload[64];
    size_t idx = 0;

    /* Protocol version. */
    payload[idx++] = AP_PROTOCOL_VERSION;

    /* Firmware version from app_header. */
    payload[idx++] = (uint8_t)((app_header.version >> 16) & 0xFF);  /* major */
    payload[idx++] = (uint8_t)((app_header.version >> 8)  & 0xFF);  /* minor */
    payload[idx++] = (uint8_t)((app_header.version >> 0)  & 0xFF);  /* patch */

    /* Device name from persistent data. */
    const pd_data_t *pd = persistent_data_get();
    const char *name = (pd != NULL) ? pd->device_name : "AttentioLight";
    uint8_t name_len = (uint8_t)strlen(name);
    payload[idx++] = name_len;
    memcpy(&payload[idx], name, name_len);
    idx += name_len;

    /* Device model. */
    uint8_t model_len = (uint8_t)strlen(DEVICE_MODEL);
    payload[idx++] = model_len;
    memcpy(&payload[idx], DEVICE_MODEL, model_len);
    idx += model_len;

    micb_respond_ok(iface, payload, (uint8_t)idx);
}

static void cmd_get_session(micb_interface_id_t iface) {
    uint8_t payload[2];
    payload[0] = (uint8_t)micb_session.mode;
    payload[1] = (uint8_t)micb_session.active_controller;

    micb_respond_ok(iface, payload, sizeof(payload));
}

static void cmd_get_metadata(micb_interface_id_t iface) {
    /*
     * Build metadata response with key-value pairs.
     * Format: [count:1] then for each: [key_len:1][key:N][val_len:1][val:M]
     * We use the same format as SETTINGS_LIST for consistency.
     */
    static uint8_t payload[AP_MAX_PAYLOAD_SIZE];
    size_t idx = 0;

    /* Firmware version string. */
    uint8_t fw_major = (uint8_t)((app_header.version >> 16) & 0xFF);
    uint8_t fw_minor = (uint8_t)((app_header.version >> 8)  & 0xFF);
    uint8_t fw_patch = (uint8_t)((app_header.version >> 0)  & 0xFF);

    char ver_str[16];
    int ver_len = 0;
    /* Simple itoa for version: "M.m.p" */
    ver_str[ver_len++] = '0' + (fw_major / 10);
    if (fw_major >= 10) {
        ver_str[0] = '0' + (fw_major / 10);
        ver_str[1] = '0' + (fw_major % 10);
        ver_len = 2;
    } else {
        ver_str[0] = '0' + fw_major;
        ver_len = 1;
    }
    ver_str[ver_len++] = '.';
    if (fw_minor >= 10) {
        ver_str[ver_len++] = '0' + (fw_minor / 10);
        ver_str[ver_len++] = '0' + (fw_minor % 10);
    } else {
        ver_str[ver_len++] = '0' + fw_minor;
    }
    ver_str[ver_len++] = '.';
    if (fw_patch >= 10) {
        ver_str[ver_len++] = '0' + (fw_patch / 10);
        ver_str[ver_len++] = '0' + (fw_patch % 10);
    } else {
        ver_str[ver_len++] = '0' + fw_patch;
    }

    /* Helper macro to add a key-value pair. */
    #define ADD_KV(key_str, val_ptr, val_sz) do {                           \
        uint8_t kl = (uint8_t)strlen(key_str);                             \
        if ((idx + 1 + kl + 1 + (val_sz)) <= AP_MAX_PAYLOAD_SIZE) {       \
            payload[idx++] = kl;                                            \
            memcpy(&payload[idx], (key_str), kl);                          \
            idx += kl;                                                      \
            payload[idx++] = (uint8_t)(val_sz);                            \
            memcpy(&payload[idx], (val_ptr), (val_sz));                    \
            idx += (val_sz);                                                \
            count++;                                                        \
        }                                                                   \
    } while (0)

    /* Reserve byte 0 for count, fill in later. */
    idx = 1;
    uint8_t count = 0;

    ADD_KV("device_model", DEVICE_MODEL, strlen(DEVICE_MODEL));
    ADD_KV("firmware_version", ver_str, (size_t)ver_len);
    ADD_KV("build_date", __DATE__, strlen(__DATE__));
    ADD_KV("platform", "STM32C071", 9);
    ADD_KV("architecture", "ARMv6-M", 7);
    ADD_KV("core_variant", "Cortex-M0+", 10);

    /* ChibiOS kernel version. */
    const char *ker = CH_KERNEL_VERSION;
    ADD_KV("chibios_kernel", ker, strlen(ker));

    #undef ADD_KV

    /* Fill in count at byte 0. */
    payload[0] = count;

    micb_respond_ok(iface, payload, (uint8_t)idx);
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
        const char *key = "loglevel";
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
    else if (key_len == 8 && memcmp(key, "loglevel", 8) == 0) {
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
    else if (key_len == 8 && memcmp(key, "loglevel", 8) == 0) {
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
            LOG_ERROR("MICB: Failed to save loglevel: %s",
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

    LOG_INFO("MICB: LOG_SET_LEVEL(%u) from %s", level,
             micb_interface_name(iface));

    log_set_level(level);
    micb_respond_ok(iface, NULL, 0);
}

/* --- DFU command --- */

static void cmd_dfu_enter(micb_interface_id_t iface) {
    LOG_INFO("MICB: DFU_ENTER from %s — rebooting to bootloader",
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

    memset(micb_send_fns, 0, sizeof(micb_send_fns));

    LOG_INFO("MICB: Initialized (mode=STANDALONE)");
}

void micb_register_interface(micb_interface_id_t iface,
                             micb_send_fn_t send_fn) {
    if (iface < MICB_IF_COUNT) {
        micb_send_fns[iface] = send_fn;
        LOG_INFO("MICB: Registered interface %s", micb_interface_name(iface));
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
    case AP_CMD_GET_STATE:      cmd_get_state(iface);           break;
    case AP_CMD_GET_CAPS:       cmd_get_caps(iface);            break;
    case AP_CMD_GET_SESSION:    cmd_get_session(iface);         break;
    case AP_CMD_GET_METADATA:   cmd_get_metadata(iface);        break;

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
