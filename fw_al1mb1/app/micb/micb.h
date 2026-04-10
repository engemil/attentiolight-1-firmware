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
 * @file    micb.h
 * @brief   Multi-Interface Control Broker (MICB).
 *
 * @details The MICB manages session state and routes Attentio Protocol (AP)
 *          commands to the appropriate handlers. It tracks which remote
 *          interface (USB, BLE, WiFi) has claimed control of the device
 *          and enforces access control for commands that require a claim.
 *
 *          The MICB operates in two control modes:
 *          - STANDALONE: Local button drives the state machine.
 *          - REMOTE: A remote interface controls the device via AP commands.
 *
 *          When a remote interface claims control, the MICB transitions the
 *          device to external control mode (via the animation engine) and
 *          routes all AP LED/power commands through the animation thread.
 */

#ifndef MICB_H
#define MICB_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "attentio_protocol.h"

/*===========================================================================*/
/* Types                                                                     */
/*===========================================================================*/

/**
 * @brief   Control modes.
 */
typedef enum {
    MICB_MODE_STANDALONE = 0,   /**< Local button controls device.         */
    MICB_MODE_REMOTE            /**< Remote interface controls device.      */
} micb_control_mode_t;

/**
 * @brief   Interface identifiers.
 */
typedef enum {
    MICB_IF_NONE = 0,           /**< No active controller.                 */
    MICB_IF_STANDALONE,         /**< Local button control.                 */
    MICB_IF_USB,                /**< USB CDC interface.                    */
    MICB_IF_BLE,                /**< Bluetooth LE (via ESP32).             */
    MICB_IF_WIFI,               /**< WiFi (via ESP32).                     */
    MICB_IF_COUNT
} micb_interface_id_t;

/**
 * @brief   Session state.
 */
typedef struct {
    micb_control_mode_t mode;               /**< Current control mode.     */
    micb_interface_id_t active_controller;  /**< Who has control.          */
} micb_session_t;

/**
 * @brief   Callback type for sending AP packet data to an interface.
 * @details The USB adapter (and future BLE/WiFi adapters) register a send
 *          callback so MICB can push responses and events back to the host.
 *
 * @param[in] data      Raw AP packet bytes to send.
 * @param[in] len       Number of bytes to send.
 */
typedef void (*micb_send_fn_t)(const uint8_t *data, size_t len);

/*===========================================================================*/
/* Public API                                                                */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Initialize the MICB module.
 * @details Sets the session to STANDALONE mode with MICB_IF_STANDALONE
 *          as the active controller. Must be called before any other
 *          MICB functions.
 */
void micb_init(void);

/**
 * @brief   Register a send callback for an interface.
 * @details Each interface adapter registers its send function so MICB
 *          can push responses and unsolicited events.
 *
 * @param[in] iface     Interface ID (must be USB, BLE, or WIFI).
 * @param[in] send_fn   Send callback function.
 */
void micb_register_interface(micb_interface_id_t iface, micb_send_fn_t send_fn);

/**
 * @brief   Process a received AP command from an interface.
 * @details This is the main entry point called by interface adapters when
 *          a complete AP packet has been parsed. The MICB:
 *          1. Checks if the command requires a claim and verifies it.
 *          2. Dispatches to the appropriate command handler.
 *          3. Builds a response packet and sends it via the interface's
 *             registered send callback.
 *
 * @param[in] iface     Source interface that sent the command.
 * @param[in] pkt       Parsed AP packet (command + payload).
 */
void micb_process_command(micb_interface_id_t iface, const ap_packet_t *pkt);

/**
 * @brief   Forward a button event to the active remote controller.
 * @details Called by the button driver callback when a button event occurs
 *          and MICB is in REMOTE mode. The event is packaged as an AP
 *          BUTTON event and sent to the active controller's interface.
 *
 *          If in STANDALONE mode, this function does nothing (the button
 *          event is handled by the standalone state machine instead).
 *
 * @param[in] event     Button event type (ap_button_event_t).
 */
void micb_forward_button_event(ap_button_event_t event);

/**
 * @brief   Get the current session state.
 *
 * @return  Pointer to the current session state (read-only).
 */
const micb_session_t *micb_get_session(void);

/**
 * @brief   Get the current control mode.
 *
 * @return  Current control mode (STANDALONE or REMOTE).
 */
micb_control_mode_t micb_get_mode(void);

/**
 * @brief   Get the active controller interface.
 *
 * @return  Active controller interface ID.
 */
micb_interface_id_t micb_get_active_controller(void);

/**
 * @brief   Check if an interface is the active controller.
 *
 * @param[in] iface     Interface to check.
 * @return  true if the interface is the active controller.
 */
bool micb_is_controller(micb_interface_id_t iface);

/**
 * @brief   Get the name string for an interface ID.
 *
 * @param[in] iface     Interface ID.
 * @return  Human-readable interface name.
 */
const char *micb_interface_name(micb_interface_id_t iface);

/**
 * @brief   Get the name string for a control mode.
 *
 * @param[in] mode      Control mode.
 * @return  Human-readable mode name.
 */
const char *micb_mode_name(micb_control_mode_t mode);

#ifdef __cplusplus
}
#endif

#endif /* MICB_H */
