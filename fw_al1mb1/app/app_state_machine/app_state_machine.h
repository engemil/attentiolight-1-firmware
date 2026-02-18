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
 * @file    app_state_machine.h
 * @brief   Application State Machine for AttentioLight1.
 *
 * @details This module implements the main application state machine that
 *          manages system states (boot, powerup, active, powerdown, off) and
 *          operational modes (solid color, brightness, blinking, etc.).
 *
 *          Architecture:
 *          - State Machine Thread: Handles input events and state transitions
 *          - Animation Thread: Handles smooth LED animations without jitter
 *
 *          Button mapping:
 *          - Short press: Mode-specific action (next color, adjust setting)
 *          - Long press release: Go to next mode
 *          - Extended press release: Turn off (powerdown)
 */

#ifndef APP_STATE_MACHINE_H
#define APP_STATE_MACHINE_H

#include "ch.h"
#include "hal.h"
#include <stdbool.h>

#include "app_state_machine_config.h"

/*===========================================================================*/
/* Data Types                                                                */
/*===========================================================================*/

/**
 * @brief   System states (top-level state machine).
 */
typedef enum {
    APP_SM_SYS_BOOT = 0,        /**< Initialization phase                   */
    APP_SM_SYS_POWERUP,         /**< Powerup animation (fade-in)            */
    APP_SM_SYS_ACTIVE,          /**< Normal operation (modes available)     */
    APP_SM_SYS_POWERDOWN,       /**< Powerdown animation (fade-out)         */
    APP_SM_SYS_OFF              /**< Off / low power mode                   */
} app_sm_system_state_t;

/**
 * @brief   Operational modes (within ACTIVE state).
 * @details These modes are cycled through with long press.
 */
typedef enum {
    APP_SM_MODE_SOLID_COLOR = 0,    /**< Static color display               */
    APP_SM_MODE_BRIGHTNESS,         /**< Brightness/intensity control       */
    APP_SM_MODE_BLINKING,           /**< Blink effect                       */
    APP_SM_MODE_PULSATION,          /**< Breathing/pulse effect             */
    APP_SM_MODE_EFFECTS,            /**< Dynamic effects (submodes)         */
    APP_SM_MODE_TRAFFIC_LIGHT,      /**< Traffic light sequence             */
    APP_SM_MODE_NIGHT_LIGHT,        /**< Dim warm night light               */
    APP_SM_MODE_COUNT               /**< Number of user-cyclable modes (7)  */
} app_sm_mode_t;

/**
 * @brief   Effects sub-modes (within EFFECTS mode).
 */
typedef enum {
    APP_SM_EFFECTS_RAINBOW = 0,     /**< Rainbow color cycle                */
    APP_SM_EFFECTS_STROBE,          /**< Strobe/flash effect                */
    APP_SM_EFFECTS_COLOR_CYCLE,     /**< Smooth color transitions           */
    APP_SM_EFFECTS_COUNT            /**< Number of effects sub-modes        */
} app_sm_effects_submode_t;

/**
 * @brief   Input events for the state machine.
 * @details Extensible for future input sources (WiFi, BLE, etc.).
 */
typedef enum {
    APP_SM_INPUT_NONE = 0,

    /* Button inputs */
    APP_SM_INPUT_BTN_SHORT,             /**< Short button press             */
    APP_SM_INPUT_BTN_LONG_START,        /**< Long press threshold reached   */
    APP_SM_INPUT_BTN_LONG_RELEASE,      /**< Released after long press      */
    APP_SM_INPUT_BTN_EXTENDED_START,    /**< Extended press threshold       */
    APP_SM_INPUT_BTN_EXTENDED_RELEASE,  /**< Released after extended press  */

    /* External control inputs */
    APP_SM_INPUT_EXT_CTRL_ENTER,        /**< Enter external control mode    */
    APP_SM_INPUT_EXT_CTRL_EXIT,         /**< Exit external control mode     */
    APP_SM_INPUT_EXT_COMMAND,           /**< External command received      */

    /* System inputs */
    APP_SM_INPUT_BOOT_COMPLETE,         /**< Boot sequence finished         */
    APP_SM_INPUT_POWERUP_COMPLETE,      /**< Powerup animation finished     */
    APP_SM_INPUT_POWERDOWN_COMPLETE,    /**< Powerdown animation finished   */
    APP_SM_INPUT_MODE_TRANSITION_COMPLETE /**< Mode transition delay done   */
} app_sm_input_t;

/**
 * @brief   State machine driver state.
 */
typedef enum {
    APP_SM_STATE_UNINIT = 0,    /**< Not initialized                        */
    APP_SM_STATE_STOPPED,       /**< Initialized but not running            */
    APP_SM_STATE_RUNNING        /**< Active and processing events           */
} app_sm_driver_state_t;

/**
 * @brief   RGB color structure.
 */
typedef struct {
    uint8_t r;                  /**< Red component (0-255)                  */
    uint8_t g;                  /**< Green component (0-255)                */
    uint8_t b;                  /**< Blue component (0-255)                 */
} app_sm_color_t;

/**
 * @brief   Mode operations interface.
 * @details Each mode implements this interface for consistent handling.
 */
typedef struct {
    const char* name;                   /**< Mode name for debugging        */
    void (*enter)(void);                /**< Called when entering mode      */
    void (*exit)(void);                 /**< Called when leaving mode       */
    void (*on_short_press)(void);       /**< Handle short press             */
    void (*on_long_start)(void);        /**< Handle long press start        */
} app_sm_mode_ops_t;

/*===========================================================================*/
/* Public API                                                                */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Initializes the application state machine.
 * @details Creates threads and internal structures. Does not start operation.
 *
 * @return  0 on success, 1 if already initialized.
 *
 * @api
 */
uint8_t app_sm_init(void);

/**
 * @brief   Starts the application state machine.
 * @details Begins processing events and transitions to POWERUP state.
 *
 * @return  0 on success, 1 if not initialized, 2 if already running.
 *
 * @api
 */
uint8_t app_sm_start(void);

/**
 * @brief   Stops the application state machine.
 * @details Stops processing and turns off LEDs.
 *
 * @return  0 on success, 1 if not running.
 *
 * @api
 */
uint8_t app_sm_stop(void);

/**
 * @brief   Processes an input event.
 * @details Thread-safe. Can be called from button callback or other contexts.
 *
 * @param[in] input     The input event to process.
 *
 * @api
 */
void app_sm_process_input(app_sm_input_t input);

/**
 * @brief   Gets the current system state.
 *
 * @return  Current system state.
 *
 * @api
 */
app_sm_system_state_t app_sm_get_system_state(void);

/**
 * @brief   Gets the current operational mode.
 * @note    Only valid when system state is ACTIVE.
 *
 * @return  Current operational mode.
 *
 * @api
 */
app_sm_mode_t app_sm_get_mode(void);

/**
 * @brief   Gets the driver state.
 *
 * @return  Current driver state (UNINIT, STOPPED, or RUNNING).
 *
 * @api
 */
app_sm_driver_state_t app_sm_get_driver_state(void);

/**
 * @brief   Checks if external control is active.
 *
 * @return  true if external control is active, false otherwise.
 *
 * @api
 */
bool app_sm_is_external_control_active(void);

/**
 * @brief   Enters external control mode.
 * @details Saves current mode and allows external commands.
 *
 * @api
 */
void app_sm_external_control_enter(void);

/**
 * @brief   Exits external control mode.
 * @details Restores previous mode.
 *
 * @api
 */
void app_sm_external_control_exit(void);

/**
 * @brief   Gets the name of a system state.
 *
 * @param[in] state     The system state.
 *
 * @return  String name of the state.
 *
 * @api
 */
const char* app_sm_system_state_name(app_sm_system_state_t state);

/**
 * @brief   Gets the name of an operational mode.
 *
 * @param[in] mode      The operational mode.
 *
 * @return  String name of the mode.
 *
 * @api
 */
const char* app_sm_mode_name(app_sm_mode_t mode);

/**
 * @brief   Gets the name of an input event.
 *
 * @param[in] input     The input event.
 *
 * @return  String name of the input.
 *
 * @api
 */
const char* app_sm_input_name(app_sm_input_t input);

#ifdef __cplusplus
}
#endif

#endif /* APP_STATE_MACHINE_H */
