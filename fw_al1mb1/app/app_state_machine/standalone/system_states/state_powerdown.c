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
 * @file    state_powerdown.c
 * @brief   Powerdown state implementation.
 *
 * @details The powerdown state shows a powerdown sequence animation,
 *          then transitions to the off state.
 */

#include "system_states.h"
#include "state_powerdown_config.h"
#include "animation_thread.h"
#include "standalone_config.h"
#include "button_driver.h"

/*===========================================================================*/
/* Local Variables                                                           */
/*===========================================================================*/

static virtual_timer_t powerdown_timer;

/*===========================================================================*/
/* Local Functions                                                           */
/*===========================================================================*/

/**
 * @brief   Timer callback when powerdown animation completes.
 */
static void powerdown_timer_cb(virtual_timer_t *vtp, void *arg) {
    (void)vtp;
    (void)arg;

    /* Signal powerdown complete */
    app_sm_process_input_isr(APP_SM_INPUT_POWERDOWN_COMPLETE);
}

/*===========================================================================*/
/* Powerdown State Implementation                                            */
/*===========================================================================*/

void state_powerdown_enter(void) {
    /* Deactivate button driver */
    button_stop();

    /* Start powerdown sequence animation (amber pulse fade) */
    anim_thread_powerdown_sequence(APP_SM_DEFAULT_BRIGHTNESS);

    /* Set a timer to transition to off state after animation completes */
    chVTObjectInit(&powerdown_timer);
    chVTSet(&powerdown_timer, TIME_MS2I(APP_SM_POWERDOWN_TOTAL_MS + 100),
            powerdown_timer_cb, NULL);
}

void state_powerdown_process(app_sm_input_t input) {
    (void)input;
    /* Nothing, ignore all inputs */
}

void state_powerdown_exit(void) {
    /* Cancel timer if still running */
    chVTReset(&powerdown_timer);
}
