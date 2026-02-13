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
 *          transitions to the startup state once initialization is complete.
 */

#include "system_states.h"
#include "../animation/animation_thread.h"

/* Debug includes */
#include "hal.h"
#include "chprintf.h"

/* Forward declaration - SDU1 is defined in usbcfg.c */
extern SerialUSBDriver SDU1;

/* Debug macro - set to 1 to enable debug output */
#define STATE_BOOT_DEBUG  1

#if STATE_BOOT_DEBUG
#define BOOT_DBG(fmt, ...) chprintf((BaseSequentialStream*)&SDU1, "[BOOT] " fmt "\r\n", ##__VA_ARGS__)
#else
#define BOOT_DBG(fmt, ...)
#endif

/*===========================================================================*/
/* Boot State Implementation                                                 */
/*===========================================================================*/

void state_boot_enter(void) {
    BOOT_DBG("state_boot_enter()");

    /* Initialize animation thread */
    BOOT_DBG("calling anim_thread_init()...");
    anim_thread_init();
    BOOT_DBG("anim_thread_init() returned");

    BOOT_DBG("calling anim_thread_start()...");
    anim_thread_start();
    BOOT_DBG("anim_thread_start() returned");

    /* Boot is instant - signal transition to startup */
    BOOT_DBG("posting STARTUP_COMPLETE input...");
    app_sm_process_input(APP_SM_INPUT_STARTUP_COMPLETE);
    BOOT_DBG("state_boot_enter() done");
}

void state_boot_process(app_sm_input_t input) {
    (void)input;
    /* Boot state processes no inputs - it's a transient state */
}

void state_boot_exit(void) {
    /* Nothing to clean up */
}
