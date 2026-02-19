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
 * @file    state_powerup.c
 * @brief   Powerup state implementation.
 *
 * @details The powerup state shows a fade-in animation, then transitions
 *          to the active state.
 */

#include "system_states.h"
#include "state_powerup_config.h"
#include "../animation/animation_thread.h"
#include "../app_state_machine_config.h"

/*===========================================================================*/
/* Local Variables                                                           */
/*===========================================================================*/

static virtual_timer_t powerup_timer;

/*===========================================================================*/
/* Local Functions                                                           */
/*===========================================================================*/

/**
 * @brief   Timer callback when powerup animation completes.
 */
static void powerup_timer_cb(virtual_timer_t *vtp, void *arg) {
    (void)vtp;
    (void)arg;

    /* Signal powerup complete */
    app_sm_process_input_isr(APP_SM_INPUT_POWERUP_COMPLETE);
}

/*===========================================================================*/
/* Powerup State Implementation                                              */
/*===========================================================================*/

void state_powerup_enter(void) {
    /* Start the multi-phase powerup sequence animation */
    anim_thread_powerup_sequence(APP_SM_DEFAULT_BRIGHTNESS);

    /* Set a timer to transition to active state after animation completes */
    chVTObjectInit(&powerup_timer);
    chVTSet(&powerup_timer, TIME_MS2I(APP_SM_POWERUP_TOTAL_MS + 100),
            powerup_timer_cb, NULL);
}

void state_powerup_process(app_sm_input_t input) {
    (void)input;
    /* Nothing, ignore all inputs */
}

void state_powerup_exit(void) {
    /* Cancel timer if still running */
    chVTReset(&powerup_timer);
}
