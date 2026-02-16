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
#include "../animation/animation_thread.h"
#include "../app_state_machine_config.h"

/*===========================================================================*/
/* Powerup State Implementation                                              */
/*===========================================================================*/

void state_powerup_enter(void) {
    /* Start fade-in animation to a white color */
    anim_thread_fade_in(255, 255, 255, APP_SM_DEFAULT_BRIGHTNESS,
                        APP_SM_POWERUP_FADE_MS);
}

void state_powerup_process(app_sm_input_t input) {
    (void)input;
    /* Powerup ignores all inputs - animation runs to completion */
}

void state_powerup_exit(void) {
    /* Nothing to clean up */
}
