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
 * @file    state_boot.c
 * @brief   Boot state implementation.
 *
 * @details The boot state handles system initialization. It immediately
 *          transitions to the powerup state once initialization is complete.
 */

#include "system_states.h"
#include "animation_thread.h"
#include "app_debug.h"


/*===========================================================================*/
/* Boot State Implementation                                                 */
/*===========================================================================*/

void state_boot_enter(void) {
    DBG_DEBUG("state_boot_enter()");

    /*
     * Boot state just signals the transition to powerup.
     */

    /* Boot is instant - signal transition to powerup */
    DBG_DEBUG("posting BOOT_COMPLETE input...");
    app_sm_process_input(APP_SM_INPUT_BOOT_COMPLETE);
    DBG_DEBUG("state_boot_enter() done");
}

void state_boot_process(app_sm_input_t input) {
    (void)input;
    /* Nothing, ignore all inputs */
}

void state_boot_exit(void) {

}
