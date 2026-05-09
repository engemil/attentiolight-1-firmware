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
#include "standalone_state.h"
#include "animation_thread.h"
#include "standalone_config.h"
#include "button_driver.h"

/*===========================================================================*/
/* External References                                                       */
/*===========================================================================*/

/* Standalone state variables are in standalone_state.c, declared in standalone_state.h */

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
    /*
     * NOTE on REMOTE mode: button events that would change mode/color
     * (SHORT_PRESS, LONG_PRESS_START, LONG_PRESS_RELEASE) are filtered out
     * upstream by `button_event_wrapper()` in main.c when MICB is in REMOTE
     * mode, so they never reach this handler. EXTENDED_START is still
     * delivered here so the user can power off the device with a 5+s press
     * even while a host has claimed control.
     */
    switch (input) {
        case APP_SM_INPUT_BTN_SHORT:
            /* Delegate to current mode */
            modes_on_short_press();
            break;

        case APP_SM_INPUT_BTN_LONG_START:
            /* Show feedback that mode change is coming */
            show_long_press_feedback();
            /* Delegate to mode for any mode-specific handling */
            modes_on_long_start();
            break;

        case APP_SM_INPUT_BTN_LONG_RELEASE:
            /* Go to next mode */
            goto_next_mode();
            break;

        case APP_SM_INPUT_BTN_EXTENDED_START:
            /* Powerdown is now triggered immediately by the main state machine */
            /* No feedback needed here */
            break;

        case APP_SM_INPUT_BTN_EXTENDED_RELEASE:
            /* Not used */
            break;

        default:
            break;
    }
}

void state_active_exit(void) {
    /* Exit current mode */
    modes_exit_current();
}
