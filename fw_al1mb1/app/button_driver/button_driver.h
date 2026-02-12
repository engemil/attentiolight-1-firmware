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

/*
 * Button Driver for ChibiOS
 *
 * A button state detection driver that detects short, long, and longest
 * button presses using interrupt-based edge detection with a dedicated
 * thread for timing and callback management.
 *
 * Features:
 * - Interrupt-driven edge detection (efficient for RTOS)
 * - Software debouncing
 * - Three press types: short (<1s), long (2-5s), longest (>=5s)
 * - Callbacks at threshold crossings AND on release
 * - Thread-safe callback invocation (not from ISR context)
 */

#ifndef BUTTON_DRIVER_H
#define BUTTON_DRIVER_H

#include "ch.h"
#include "hal.h"
#include "button_driver_config.h"

/*===========================================================================*/
/* Driver Data Types                                                         */
/*===========================================================================*/

/**
 * @brief   Button event types.
 * @details Events are fired at threshold crossings and on release.
 */
typedef enum {
    BTN_EVT_NONE = 0,                /**< No event (internal use)        */
    BTN_EVT_SHORT_PRESS,             /**< Released after < 1s            */
    BTN_EVT_LONG_PRESS_START,        /**< Held for >= 2s (still held)    */
    BTN_EVT_LONG_PRESS_RELEASE,      /**< Released after 2-5s            */
    BTN_EVT_LONGEST_PRESS_START,     /**< Held for >= 5s (still held)    */
    BTN_EVT_LONGEST_PRESS_RELEASE    /**< Released after >= 5s           */
} button_event_t;

/**
 * @brief   Button driver state.
 */
typedef enum {
    BTN_STATE_UNINIT = 0,            /**< Driver not initialized         */
    BTN_STATE_STOPPED,               /**< Initialized but not running    */
    BTN_STATE_RUNNING                /**< Driver active and monitoring   */
} button_driver_state_t;

/**
 * @brief   Button callback function type.
 * @details Called from button thread context (not ISR), safe to use
 *          RTOS functions. Keep callback short to maintain responsiveness.
 *
 * @param[in] event     The button event that occurred.
 */
typedef void (*button_callback_t)(button_event_t event);

/*===========================================================================*/
/* Driver Public API                                                         */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Initializes the button driver.
 * @details Sets up internal state, creates the button monitoring thread.
 *          Does NOT enable interrupts - call button_start() for that.
 *
 * @param[in] button_line   GPIO line for the button (e.g., LINE_USER_BUTTON).
 * @param[in] active_state  Button active state: PAL_LOW for active-low
 *                          (externally pulled up), PAL_HIGH for active-high
 *                          (externally pulled down).
 *
 * @return  0 on success, 1 if already initialized.
 *
 * @api
 */
uint8_t button_init(ioline_t button_line, uint8_t active_state);

/**
 * @brief   Starts the button driver.
 * @details Enables PAL interrupt and begins button monitoring.
 *
 * @return  0 on success, 1 if not initialized, 2 if already running.
 *
 * @api
 */
uint8_t button_start(void);

/**
 * @brief   Stops the button driver.
 * @details Disables PAL interrupt. Thread continues but remains idle.
 *
 * @return  0 on success, 1 if not running.
 *
 * @api
 */
uint8_t button_stop(void);

/**
 * @brief   Registers a callback for button events.
 * @details Only one callback is supported at a time. Setting a new callback
 *          replaces the previous one. Set to NULL to unregister.
 *
 * @param[in] callback  Function to call on button events, or NULL.
 *
 * @return  0 on success.
 *
 * @api
 */
uint8_t button_register_callback(button_callback_t callback);

/**
 * @brief   Returns current button pressed state.
 * @details Reads the debounced button state, not the raw GPIO.
 *
 * @return  true if button is currently pressed, false otherwise.
 *
 * @api
 */
bool button_is_pressed(void);

/**
 * @brief   Returns the current driver state.
 *
 * @return  Current driver state (UNINIT, STOPPED, or RUNNING).
 *
 * @api
 */
button_driver_state_t button_get_state(void);

/**
 * @brief   Returns the event name as a string.
 * @details Useful for debugging and logging.
 *
 * @param[in] event     The button event.
 *
 * @return  String representation of the event.
 *
 * @api
 */
const char* button_event_name(button_event_t event);

#ifdef __cplusplus
}
#endif

#endif /* BUTTON_DRIVER_H */
