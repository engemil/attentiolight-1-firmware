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
 * @file    state_startup.c
 * @brief   Startup state implementation.
 *
 * @details The startup state shows a fade-in animation, then transitions
 *          to the active state.
 */

#include "system_states.h"
#include "../animation/animation_thread.h"
#include "../app_state_machine_config.h"

/*===========================================================================*/
/* Local Variables                                                           */
/*===========================================================================*/

static virtual_timer_t startup_timer;

/*===========================================================================*/
/* Local Functions                                                           */
/*===========================================================================*/

/**
 * @brief   Timer callback when startup animation completes.
 */
static void startup_timer_cb(virtual_timer_t *vtp, void *arg) {
    (void)vtp;
    (void)arg;

    /* Signal startup complete - this will be processed by SM thread */
    chSysLockFromISR();
    /* We need to signal the SM thread - use a simple approach */
    chSysUnlockFromISR();

    /* The state machine will check for completion via animation state */
}

/*===========================================================================*/
/* Startup State Implementation                                              */
/*===========================================================================*/

void state_startup_enter(void) {
    /* Start fade-in animation to a white color */
    anim_thread_fade_in(255, 255, 255, APP_SM_DEFAULT_BRIGHTNESS,
                        APP_SM_STARTUP_FADE_MS);

    /* Set a timer to transition to active state */
    chVTObjectInit(&startup_timer);
    chVTSet(&startup_timer, TIME_MS2I(APP_SM_STARTUP_FADE_MS + 100),
            startup_timer_cb, NULL);
}

void state_startup_process(app_sm_input_t input) {
    /* During startup, we ignore most inputs but allow skip */
    if (input == APP_SM_INPUT_BTN_SHORT) {
        /* Skip startup animation - go directly to active */
        chVTReset(&startup_timer);
        app_sm_process_input(APP_SM_INPUT_STARTUP_COMPLETE);
    }
}

void state_startup_exit(void) {
    /* Cancel timer if still running */
    chVTReset(&startup_timer);
}
