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
 * @file    app_state_machine.c
 * @brief   Application State Machine implementation.
 *
 * @details This is the core state machine that manages system states and
 *          routes input events to the appropriate handlers.
 */

#include "app_state_machine.h"
#include "system_states/system_states.h"
#include "modes/modes.h"
#include "animation/animation_thread.h"
#include "button_driver.h"

/* Debug support */
#include "app_debug.h"

/*===========================================================================*/
/* App State Machine Macros                                                  */
/*===========================================================================*/

/**
 * @brief   String macros for App State Machine.
 * @details Used for consistent naming across button_driver and app_state_machine.
 */
#define APP_SM_UNKNOWN              "UNKNOWN"


/*===========================================================================*/
/* Global Variables (exported to other modules)                              */
/*===========================================================================*/

/**
 * @brief   Current operational mode.
 */
app_sm_mode_t current_mode = APP_SM_DEFAULT_MODE;

/**
 * @brief   External control active flag.
 */
bool external_control_active = false;

/**
 * @brief   Saved mode before external control.
 */
app_sm_mode_t saved_mode_before_external = APP_SM_DEFAULT_MODE;

/**
 * @brief   Global brightness level.
 */
uint8_t global_brightness = APP_SM_DEFAULT_BRIGHTNESS;

/**
 * @brief   Global color index (0-11, indexes into color palette).
 */
uint8_t global_color_index = APP_SM_DEFAULT_COLOR_INDEX;

/*===========================================================================*/
/* Local Variables                                                           */
/*===========================================================================*/

/**
 * @brief   Current system state.
 */
static volatile app_sm_system_state_t system_state = APP_SM_SYS_BOOT;

/**
 * @brief   Driver state.
 */
static volatile app_sm_driver_state_t driver_state = APP_SM_STATE_UNINIT;

/**
 * @brief   State machine thread working area.
 */
static THD_WORKING_AREA(wa_sm_thread, APP_SM_THREAD_WA_SIZE);

/**
 * @brief   Thread reference.
 */
static thread_t* sm_thread_ref = NULL;

/**
 * @brief   Input event mailbox.
 */
static mailbox_t input_mailbox;

/**
 * @brief   Mailbox buffer.
 */
static msg_t input_mailbox_buffer[APP_SM_INPUT_QUEUE_SIZE];

/*===========================================================================*/
/* Name Lookup Tables                                                        */
/*===========================================================================*/

static const char* system_state_names[] = {
    "BOOT",
    "POWERUP",
    "ACTIVE",
    "POWERDOWN",
    "OFF"
};

static const char* mode_names[] = {
    "Solid Color",
    "Brightness",
    "Blinking",
    "Pulsation",
    "Animation",
    "Traffic Light",
    "Night Light"
};

static const char* input_names[] = {
    "NONE",
    BTN_EVT_NAME_SHORT_PRESS,
    BTN_EVT_NAME_LONG_PRESS_START,
    BTN_EVT_NAME_LONG_PRESS_RELEASE,
    BTN_EVT_NAME_EXTENDED_PRESS_START,
    BTN_EVT_NAME_EXTENDED_PRESS_RELEASE,
    "EXT_CTRL_ENTER",
    "EXT_CTRL_EXIT",
    "EXT_COMMAND",
    "BOOT_COMPLETE",
    "POWERUP_COMPLETE",
    "POWERDOWN_COMPLETE"
};

/*===========================================================================*/
/* Local Functions                                                           */
/*===========================================================================*/

/**
 * @brief   Button event callback for state machine.
 * @details Routes button events to the state machine input queue.
 *          Called from button thread context (safe for RTOS functions).
 */
static void on_button_event(button_event_t event) {
    
    DBG_DEBUG("BTN EVENT: %s", button_event_name(event));

    switch (event) {
    case BTN_EVT_SHORT_PRESS:
        app_sm_process_input(APP_SM_INPUT_BTN_SHORT);
        break;
    case BTN_EVT_LONG_PRESS_START:
        app_sm_process_input(APP_SM_INPUT_BTN_LONG_START);
        break;
    case BTN_EVT_LONG_PRESS_RELEASE:
        app_sm_process_input(APP_SM_INPUT_BTN_LONG_RELEASE);
        break;
    case BTN_EVT_EXTENDED_PRESS_START:
        app_sm_process_input(APP_SM_INPUT_BTN_EXTENDED_START);
        break;
    case BTN_EVT_EXTENDED_PRESS_RELEASE:
        app_sm_process_input(APP_SM_INPUT_BTN_EXTENDED_RELEASE);
        break;
    default:
        break;
    }
    
}

/**
 * @brief   Initializes the button driver and registers the callback.
 * @details Called from state_boot_enter() to set up button handling.
 */
void app_sm_init_button(void) {
    button_init(LINE_USER_BUTTON, PAL_LOW);
    button_register_callback(on_button_event);
}

/**
 * @brief   Transitions to a new system state.
 */
static void transition_to_state(app_sm_system_state_t new_state) {
    app_sm_system_state_t old_state = system_state;
    DBG_UNUSED(old_state);

    DBG_DEBUG("SM EXIT state: %s", system_state_names[old_state]);

    /* Exit current state */
    switch (system_state) {
        case APP_SM_SYS_BOOT:
            state_boot_exit();
            break;
        case APP_SM_SYS_POWERUP:
            state_powerup_exit();
            break;
        case APP_SM_SYS_ACTIVE:
            state_active_exit();
            break;
        case APP_SM_SYS_POWERDOWN:
            state_powerdown_exit();
            break;
        case APP_SM_SYS_OFF:
            state_off_exit();
            break;
    }

    DBG_INFO("SM %s -> %s", system_state_names[old_state], system_state_names[new_state]);

    /* Update state */
    system_state = new_state;

    DBG_DEBUG("SM ENTER state: %s", system_state_names[new_state]);

    /* Enter new state */
    switch (new_state) {
        case APP_SM_SYS_BOOT:
            state_boot_enter();
            break;
        case APP_SM_SYS_POWERUP:
            state_powerup_enter();
            break;
        case APP_SM_SYS_ACTIVE:
            state_active_enter();
            break;
        case APP_SM_SYS_POWERDOWN:
            state_powerdown_enter();
            break;
        case APP_SM_SYS_OFF:
            state_off_enter();
            break;
    }
}

/**
 * @brief   Processes an input in the current state.
 */
static void process_input_internal(app_sm_input_t input) {
    /* Handle state transitions based on input */
    switch (system_state) {
        case APP_SM_SYS_BOOT:
            /* Boot immediately transitions to powerup */
            if (input == APP_SM_INPUT_BOOT_COMPLETE) {
                transition_to_state(APP_SM_SYS_POWERUP);
            }
            break;

        case APP_SM_SYS_POWERUP:
            if (input == APP_SM_INPUT_POWERUP_COMPLETE) {
                transition_to_state(APP_SM_SYS_ACTIVE);
            }
            break;

        case APP_SM_SYS_ACTIVE:
            if (input == APP_SM_INPUT_BTN_EXTENDED_START) {
                /* Initiate powerdown */
                transition_to_state(APP_SM_SYS_POWERDOWN);
            } else {
                /* Delegate to active state handler */
                state_active_process(input);
            }
            break;

        case APP_SM_SYS_POWERDOWN:
            if (input == APP_SM_INPUT_POWERDOWN_COMPLETE) {
                transition_to_state(APP_SM_SYS_OFF);
            }
            /* Ignore other inputs during powerdown */
            break;

        case APP_SM_SYS_OFF:
            /* Any button press wakes up */
            if (input == APP_SM_INPUT_BTN_LONG_START) {
                transition_to_state(APP_SM_SYS_POWERUP);
            }
            break;
    }
}

/**
 * @brief   State machine thread function.
 */
static THD_FUNCTION(sm_thread_func, arg) {
    (void)arg;
    chRegSetThreadName("state_machine_thread");

    DBG_DEBUG("SM thread started");

    /* Enter boot state */
    transition_to_state(APP_SM_SYS_BOOT);

    while (!chThdShouldTerminateX()) {
        msg_t msg;

        /* Wait for input event */
        if (chMBFetchTimeout(&input_mailbox, &msg, TIME_MS2I(100)) == MSG_OK) {
            app_sm_input_t input = (app_sm_input_t)msg;
            DBG_DEBUG("SM INPUT: %s (state=%s)", input_names[input], system_state_names[system_state]);
            process_input_internal(input);
        }
    }
    DBG_DEBUG("SM thread terminating");
}

/*===========================================================================*/
/* Public API                                                                */
/*===========================================================================*/

uint8_t app_sm_init(void) {
    if (driver_state != APP_SM_STATE_UNINIT) {
        DBG_ERROR("SM init failed - already initialized");
        return 1;
    }

    /* Initialize mailbox */
    chMBObjectInit(&input_mailbox, input_mailbox_buffer, APP_SM_INPUT_QUEUE_SIZE);

    /* Initialize modes */
    modes_init();

    driver_state = APP_SM_STATE_STOPPED;
    DBG_DEBUG("SM init OK");
    return 0;
}

uint8_t app_sm_start(void) {
    if (driver_state == APP_SM_STATE_UNINIT) {
        DBG_ERROR("SM start failed - not initialized");
        return 1;
    }
    if (driver_state == APP_SM_STATE_RUNNING) {
        DBG_WARN("SM start - already running");
        return 2;
    }

    /* Create state machine thread */
    sm_thread_ref = chThdCreateStatic(wa_sm_thread, sizeof(wa_sm_thread),
                                      APP_SM_THREAD_PRIORITY,
                                      sm_thread_func, NULL);

    driver_state = APP_SM_STATE_RUNNING;
    DBG_DEBUG("SM started");
    return 0;
}

uint8_t app_sm_stop(void) {
    if (driver_state != APP_SM_STATE_RUNNING) {
        return 1;
    }

    /* Request thread termination */
    chThdTerminate(sm_thread_ref);
    chThdWait(sm_thread_ref);
    sm_thread_ref = NULL;

    /* Stop animation thread */
    anim_thread_stop();

    driver_state = APP_SM_STATE_STOPPED;
    return 0;
}

void app_sm_process_input(app_sm_input_t input) {
    if (driver_state != APP_SM_STATE_RUNNING) {
        return;
    }

    /* Post input to mailbox (non-blocking) */
    chMBPostTimeout(&input_mailbox, (msg_t)input, TIME_IMMEDIATE);
}

void app_sm_process_input_isr(app_sm_input_t input) {
    chSysLockFromISR();
    chMBPostI(&input_mailbox, (msg_t)input);
    chSysUnlockFromISR();
}

app_sm_system_state_t app_sm_get_system_state(void) {
    return system_state;
}

app_sm_mode_t app_sm_get_mode(void) {
    return current_mode;
}

app_sm_driver_state_t app_sm_get_driver_state(void) {
    return driver_state;
}

bool app_sm_is_external_control_active(void) {
    return external_control_active;
}

void app_sm_external_control_enter(void) {
    app_sm_process_input(APP_SM_INPUT_EXT_CTRL_ENTER);
}

void app_sm_external_control_exit(void) {
    app_sm_process_input(APP_SM_INPUT_EXT_CTRL_EXIT);
}

const char* app_sm_system_state_name(app_sm_system_state_t state) {
    if (state <= APP_SM_SYS_OFF) {
        return system_state_names[state];
    }
    return APP_SM_UNKNOWN;
}

const char* app_sm_mode_name(app_sm_mode_t mode) {
    if (mode < APP_SM_MODE_COUNT) {
        return mode_names[mode];
    }
    return APP_SM_UNKNOWN;
}

const char* app_sm_input_name(app_sm_input_t input) {
    /* Note that APP_SM_INPUT_POWERDOWN_COMPLETE is the last element in app_sm_input_t */
    if (input <= APP_SM_INPUT_POWERDOWN_COMPLETE) {
        return input_names[input];
    }
    return APP_SM_UNKNOWN;
}
