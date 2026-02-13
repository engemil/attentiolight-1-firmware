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
 * @file    state_shutdown.c
 * @brief   Shutdown state implementation.
 *
 * @details The shutdown state shows a fade-out animation, then transitions
 *          to the off state.
 */

#include "system_states.h"
#include "../animation/animation_thread.h"
#include "../app_state_machine_config.h"

/*===========================================================================*/
/* Local Variables                                                           */
/*===========================================================================*/

static virtual_timer_t shutdown_timer;

/*===========================================================================*/
/* Local Functions                                                           */
/*===========================================================================*/

/**
 * @brief   Timer callback when shutdown animation completes.
 */
static void shutdown_timer_cb(virtual_timer_t *vtp, void *arg) {
    (void)vtp;
    (void)arg;

    /* Signal shutdown complete */
    chSysLockFromISR();
    chSysUnlockFromISR();
}

/*===========================================================================*/
/* Shutdown State Implementation                                             */
/*===========================================================================*/

void state_shutdown_enter(void) {
    /* Start fade-out animation */
    anim_thread_fade_out(APP_SM_SHUTDOWN_FADE_MS);

    /* Set a timer to transition to off state */
    chVTObjectInit(&shutdown_timer);
    chVTSet(&shutdown_timer, TIME_MS2I(APP_SM_SHUTDOWN_FADE_MS + 100),
            shutdown_timer_cb, NULL);
}

void state_shutdown_process(app_sm_input_t input) {
    (void)input;
    /* During shutdown, we ignore all inputs */
}

void state_shutdown_exit(void) {
    /* Cancel timer if still running */
    chVTReset(&shutdown_timer);
}
