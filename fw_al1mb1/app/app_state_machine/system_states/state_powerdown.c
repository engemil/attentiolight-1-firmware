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
 * @details The powerdown state shows a fade-out animation, then transitions
 *          to the off state.
 */

#include "system_states.h"
#include "../animation/animation_thread.h"
#include "../app_state_machine_config.h"
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

    /* Start fade-out animation */
    anim_thread_fade_out(APP_SM_POWERDOWN_FADE_MS);

    /* Set a timer to transition to off state */
    chVTObjectInit(&powerdown_timer);
    chVTSet(&powerdown_timer, TIME_MS2I(APP_SM_POWERDOWN_FADE_MS + 100),
            powerdown_timer_cb, NULL);
}

void state_powerdown_process(app_sm_input_t input) {
    (void)input;
    /* During powerdown, we ignore all inputs */
}

void state_powerdown_exit(void) {
    /* Cancel timer if still running */
    chVTReset(&powerdown_timer);
}
