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
 * @file    state_off.c
 * @brief   Off state implementation.
 *
 * @details The off state keeps LEDs off and waits for user input to wake up.
 *
 */

/* TO DO: Implement low-power mode (STM32 Stop/Standby mode) to reduce power
 *        consumption while in off state. Configure wake-up via button EXTI
*/

#include "system_states.h"
#include "animation_thread.h"
#include "button_driver.h"

/*===========================================================================*/
/* Off State Implementation                                                  */
/*===========================================================================*/

void state_off_enter(void) {
    /* Activate button driver to allow wake-up */
    button_start();

    /* Ensure LEDs are off */
    anim_thread_off();
}

void state_off_process(app_sm_input_t input) {
    (void)input;
    /* Nothing, ignore all inputs */
}

void state_off_exit(void) {

}
