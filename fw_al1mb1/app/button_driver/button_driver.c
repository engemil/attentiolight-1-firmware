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
 * Implementation of interrupt-driven button state detection with
 * software debouncing and press duration classification.
 */

#include "button_driver.h"

/*===========================================================================*/
/* Driver Local Definitions                                                  */
/*===========================================================================*/

/**
 * @brief   Internal button state machine states.
 */
typedef enum {
    BTN_SM_IDLE = 0,            /**< Button not pressed                     */
    BTN_SM_DEBOUNCING_PRESS,    /**< Waiting for press debounce             */
    BTN_SM_PRESSED,             /**< Button pressed, timing in progress     */
    BTN_SM_LONG_DETECTED,       /**< Long press threshold crossed           */
    BTN_SM_EXTENDED_DETECTED,   /**< Extended press threshold crossed       */
    BTN_SM_DEBOUNCING_RELEASE   /**< Waiting for release debounce           */
} btn_sm_state_t;

/*===========================================================================*/
/* Driver Local Variables                                                    */
/*===========================================================================*/

/**
 * @brief   Button thread working area.
 */
static THD_WORKING_AREA(wa_button_thread, BTN_THREAD_WA_SIZE);

/**
 * @brief   Button thread pointer.
 */
static thread_t *btn_thread_ptr = NULL;

/**
 * @brief   Binary semaphore for ISR to thread signaling.
 */
static binary_semaphore_t btn_sem;

/**
 * @brief   Mutex for protecting shared state.
 */
static mutex_t btn_mtx;

/**
 * @brief   Driver state.
 */
static volatile button_driver_state_t driver_state = BTN_STATE_UNINIT;

/**
 * @brief   Internal state machine state.
 */
static volatile btn_sm_state_t sm_state = BTN_SM_IDLE;

/**
 * @brief   Timestamp when button was pressed.
 */
static systime_t press_start_time = 0;

/**
 * @brief   Last edge timestamp (for debouncing).
 */
static volatile systime_t last_edge_time = 0;

/**
 * @brief   Debounced button pressed state.
 */
static volatile bool btn_pressed = false;

/**
 * @brief   User callback function.
 */
static button_callback_t user_callback = NULL;

/**
 * @brief   Button GPIO line (set at init).
 */
static ioline_t btn_line = 0;

/**
 * @brief   Button active state (set at init).
 */
static uint8_t btn_active_state = PAL_LOW;

/*===========================================================================*/
/* Driver Local Functions                                                    */
/*===========================================================================*/

/**
 * @brief   Reads the raw button state, with inline to reduce call overhead in 
 *          critical paths.
 * @return  true if button is pressed (active), false otherwise.
 */
static inline bool read_button_raw(void) {
    return (palReadLine(btn_line) == btn_active_state);
}

/**
 * @brief   Invokes user callback if registered.
 * @param[in] event     Event to pass to callback.
 */
static void invoke_callback(button_event_t event) {
    button_callback_t cb;

    /* Get callback pointer under lock */
    chMtxLock(&btn_mtx);
    cb = user_callback;
    chMtxUnlock(&btn_mtx);

    /* Invoke outside of lock to avoid deadlocks */
    if (cb != NULL) {
        cb(event);
    }
}

/**
 * @brief   Calculates elapsed time since press start.
 * @return  Elapsed time in milliseconds.
 */
static inline uint32_t get_press_duration_ms(void) {
    systime_t now = chVTGetSystemTime();
    sysinterval_t elapsed = chTimeDiffX(press_start_time, now);
    return (uint32_t)TIME_I2MS(elapsed);
}

/**
 * @brief   PAL callback for button edge detection.
 * @details Called from interrupt context on both rising and falling edges.
 */
static void button_pal_cb(void *arg) {
    (void)arg;

    chSysLockFromISR();
    last_edge_time = chVTGetSystemTimeX();
    chBSemSignalI(&btn_sem);
    chSysUnlockFromISR();
}

/**
 * @brief   Button monitoring thread function.
 */
static THD_FUNCTION(button_thread, arg) {
    (void)arg;

    chRegSetThreadName("button");

    while (true) {
        msg_t result;
        sysinterval_t timeout;

        /* Determine timeout based on state */
        if (sm_state == BTN_SM_IDLE) {
            /* Idle state: wait indefinitely for edge */
            timeout = TIME_INFINITE;
        } else if (sm_state == BTN_SM_DEBOUNCING_PRESS ||
                   sm_state == BTN_SM_DEBOUNCING_RELEASE) {
            /* Debouncing: wait for debounce period */
            timeout = TIME_MS2I(BTN_DEBOUNCE_MS);
        } else {
            /* Pressed state: poll for threshold checking */
            timeout = TIME_MS2I(BTN_POLL_INTERVAL_MS);
        }

        /* Wait for signal or timeout */
        result = chBSemWaitTimeout(&btn_sem, timeout);

        /* Check if driver is stopped */
        if (driver_state != BTN_STATE_RUNNING) {
            /* Reset state machine when stopped */
            sm_state = BTN_SM_IDLE;
            btn_pressed = false;
            continue;
        }

        /* State machine processing */
        switch (sm_state) {
        case BTN_SM_IDLE:
            /* Waiting for press */
            if (result == MSG_OK) {
                /* Edge detected, start debounce */
                sm_state = BTN_SM_DEBOUNCING_PRESS;
            }
            break;

        case BTN_SM_DEBOUNCING_PRESS:
            if (result == MSG_TIMEOUT) {
                /* Debounce period elapsed, read stable state */
                if (read_button_raw()) {
                    /* Button confirmed pressed */
                    btn_pressed = true;
                    press_start_time = chVTGetSystemTime();
                    sm_state = BTN_SM_PRESSED;
                } else {
                    /* False trigger, return to idle */
                    sm_state = BTN_SM_IDLE;
                }
            }
            /* If MSG_OK (new edge), stay in debouncing - reset timer */
            break;

        case BTN_SM_PRESSED:
            /* Check for release or threshold crossing */
            if (!read_button_raw()) {
                /* Button released, start release debounce */
                sm_state = BTN_SM_DEBOUNCING_RELEASE;
            } else {
                /* Still pressed, check for long threshold */
                uint32_t duration = get_press_duration_ms();
                if (duration >= BTN_LONG_MIN_MS) {
                    /* Long press threshold reached */
                    sm_state = BTN_SM_LONG_DETECTED;
                    invoke_callback(BTN_EVT_LONG_PRESS_START);
                }
            }
            break;

        case BTN_SM_LONG_DETECTED:
            /* Check for release or extended threshold */
            if (!read_button_raw()) {
                /* Button released, start release debounce */
                sm_state = BTN_SM_DEBOUNCING_RELEASE;
            } else {
                /* Still pressed, check for extended threshold */
                uint32_t duration = get_press_duration_ms();
                if (duration >= BTN_EXTENDED_MIN_MS) {
                    /* Extended press threshold reached */
                    sm_state = BTN_SM_EXTENDED_DETECTED;
                    invoke_callback(BTN_EVT_EXTENDED_PRESS_START);
                }
            }
            break;

        case BTN_SM_EXTENDED_DETECTED:
            /* Just waiting for release now */
            if (!read_button_raw()) {
                /* Button released, start release debounce */
                sm_state = BTN_SM_DEBOUNCING_RELEASE;
            }
            break;

        case BTN_SM_DEBOUNCING_RELEASE:
            if (result == MSG_TIMEOUT) {
                /* Debounce period elapsed, read stable state */
                if (!read_button_raw()) {
                    /* Button confirmed released - determine event type */
                    uint32_t duration = get_press_duration_ms();
                    btn_pressed = false;

                    if (duration < BTN_SHORT_MAX_MS) {
                        /* Short press */
                        invoke_callback(BTN_EVT_SHORT_PRESS);
                    } else if (duration >= BTN_EXTENDED_MIN_MS) {
                        /* Extended press release */
                        invoke_callback(BTN_EVT_EXTENDED_PRESS_RELEASE);
                    } else if (duration >= BTN_LONG_MIN_MS) {
                        /* Long press release */
                        invoke_callback(BTN_EVT_LONG_PRESS_RELEASE);
                    }
                    /* Duration between SHORT_MAX and LONG_MIN is cancelled */

                    sm_state = BTN_SM_IDLE;
                } else {
                    /* False release, button still pressed */
                    /* Return to appropriate pressed state based on duration */
                    uint32_t duration = get_press_duration_ms();
                    if (duration >= BTN_EXTENDED_MIN_MS) {
                        sm_state = BTN_SM_EXTENDED_DETECTED;
                    } else if (duration >= BTN_LONG_MIN_MS) {
                        sm_state = BTN_SM_LONG_DETECTED;
                    } else {
                        sm_state = BTN_SM_PRESSED;
                    }
                }
            }
            /* If MSG_OK (new edge), stay in debouncing - reset timer */
            break;

        default:
            /* Unknown state, reset to idle */
            sm_state = BTN_SM_IDLE;
            btn_pressed = false;
            break;
        }
    }
}

/*===========================================================================*/
/* Driver Public Functions                                                   */
/*===========================================================================*/

uint8_t button_init(ioline_t button_line, uint8_t active_state) {
    /* Check if already initialized */
    if (driver_state != BTN_STATE_UNINIT) {
        return 1;
    }

    /* Store configuration */
    btn_line = button_line;
    btn_active_state = active_state;

    /* Initialize synchronization primitives */
    chBSemObjectInit(&btn_sem, true);  /* Start taken (not signaled) */
    chMtxObjectInit(&btn_mtx);

    /* Initialize state */
    sm_state = BTN_SM_IDLE;
    btn_pressed = false;
    user_callback = NULL;

    /* Create button thread */
    btn_thread_ptr = chThdCreateStatic(wa_button_thread,
                                       sizeof(wa_button_thread),
                                       BTN_THREAD_PRIORITY,
                                       button_thread,
                                       NULL);

    driver_state = BTN_STATE_STOPPED;

    return 0;
}

uint8_t button_start(void) {
    /* Check if initialized */
    if (driver_state == BTN_STATE_UNINIT) {
        return 1;
    }

    /* Check if already running */
    if (driver_state == BTN_STATE_RUNNING) {
        return 2;
    }

    /* Reset state machine */
    sm_state = BTN_SM_IDLE;
    btn_pressed = false;

    /* Enable PAL event on both edges */
    palEnableLineEvent(btn_line, PAL_EVENT_MODE_BOTH_EDGES);
    palSetLineCallback(btn_line, button_pal_cb, NULL);

    driver_state = BTN_STATE_RUNNING;

    /* Signal thread to wake up from infinite wait */
    chBSemSignal(&btn_sem);

    return 0;
}

uint8_t button_stop(void) {
    /* Check if running */
    if (driver_state != BTN_STATE_RUNNING) {
        return 1;
    }

    /* Disable PAL event */
    palDisableLineEvent(btn_line);

    driver_state = BTN_STATE_STOPPED;

    /* Signal thread to process state change */
    chBSemSignal(&btn_sem);

    return 0;
}

uint8_t button_register_callback(button_callback_t callback) {
    chMtxLock(&btn_mtx);
    user_callback = callback;
    chMtxUnlock(&btn_mtx);

    return 0;
}

bool button_is_pressed(void) {
    return btn_pressed;
}

button_driver_state_t button_get_state(void) {
    return driver_state;
}

const char* button_event_name(button_event_t event) {
    switch (event) {
    case BTN_EVT_NONE:
        return BTN_EVT_NAME_NONE;
    case BTN_EVT_SHORT_PRESS:
        return BTN_EVT_NAME_SHORT_PRESS;
    case BTN_EVT_LONG_PRESS_START:
        return BTN_EVT_NAME_LONG_PRESS_START;
    case BTN_EVT_LONG_PRESS_RELEASE:
        return BTN_EVT_NAME_LONG_PRESS_RELEASE;
    case BTN_EVT_EXTENDED_PRESS_START:
        return BTN_EVT_NAME_EXTENDED_PRESS_START;
    case BTN_EVT_EXTENDED_PRESS_RELEASE:
        return BTN_EVT_NAME_EXTENDED_PRESS_RELEASE;
    default:
        return BNT_EVT_UNKNOWN_EVENT;
    }
}
