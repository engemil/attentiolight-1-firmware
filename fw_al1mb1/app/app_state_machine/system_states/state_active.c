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
 * @file    state_active.c
 * @brief   Active state implementation.
 *
 * @details The active state is the main operational state where the user
 *          interacts with different modes via button presses.
 */

#include "system_states.h"
#include "modes.h"
#include "animation_thread.h"
#include "app_state_machine_config.h"
#include "button_driver.h"

/*===========================================================================*/
/* External References                                                       */
/*===========================================================================*/

/* These are managed by app_state_machine.c */
extern app_sm_mode_t current_mode;
extern bool external_control_active;
extern app_sm_mode_t saved_mode_before_external;

/*===========================================================================*/
/* Local Functions                                                           */
/*===========================================================================*/

/**
 * @brief   Transitions to the next mode.
 */
static void goto_next_mode(void) {
    /* Exit current mode */
    modes_exit_current();

    /* Advance to next mode */
    current_mode = (app_sm_mode_t)((current_mode + 1) % APP_SM_MODE_COUNT);

    /* Enter new mode */
    modes_enter_current();
}

/**
 * @brief   Shows visual feedback for long press threshold.
 */
static void show_long_press_feedback(void) {
#if APP_SM_LONG_PRESS_FEEDBACK
    /* Quick white flash to indicate mode change is coming */
    anim_thread_flash_feedback(255, 255, 255, APP_SM_MODE_FEEDBACK_MS);
#endif
}

/*===========================================================================*/
/* Active State Implementation                                               */
/*===========================================================================*/

void state_active_enter(void) {
    /* Activate button driver */
    button_start();

    /* Enter the current mode (or default mode) */
    modes_enter_current();
}

void state_active_process(app_sm_input_t input) {
    switch (input) {
        case APP_SM_INPUT_BTN_SHORT:
            if (!external_control_active) {
                /* Delegate to current mode */
                modes_on_short_press();
            }
            break;

        case APP_SM_INPUT_BTN_LONG_START:
            if (!external_control_active) {
                /* Show feedback that mode change is coming */
                show_long_press_feedback();
                /* Delegate to mode for any mode-specific handling */
                modes_on_long_start();
            }
            break;

        case APP_SM_INPUT_BTN_LONG_RELEASE:
            if (!external_control_active) {
                /* Go to next mode */
                goto_next_mode();
            }
            break;

        case APP_SM_INPUT_BTN_EXTENDED_START:
            /* Powerdown is now triggered immediately by the main state machine */
            /* No feedback needed here */
            break;

        case APP_SM_INPUT_BTN_EXTENDED_RELEASE:
            /* Not used */
            break;

        case APP_SM_INPUT_EXT_CTRL_ENTER:
            if (!external_control_active) {
                saved_mode_before_external = current_mode;
                external_control_active = true;
                /* External control mode doesn't exit current mode visually */
            }
            break;

        case APP_SM_INPUT_EXT_CTRL_EXIT:
            if (external_control_active) {
                external_control_active = false;
                /* Restore previous mode's animation */
                modes_enter_current();
            }
            break;

        default:
            break;
    }
}

void state_active_exit(void) {
    /* Exit current mode */
    modes_exit_current();
    external_control_active = false;
}
