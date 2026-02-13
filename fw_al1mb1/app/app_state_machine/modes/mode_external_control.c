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
 * @file    mode_external_control.c
 * @brief   External control mode implementation.
 *
 * @details Placeholder mode for external control (WiFi, BLE, etc.).
 *          This mode can be entered from any other mode and will be
 *          controlled by external commands.
 *
 * @note    This is a stub implementation. Full implementation will be
 *          added when external interfaces are integrated.
 */

#include "modes.h"
#include "../animation/animation_thread.h"
#include "../app_state_machine_config.h"
#include "app_debug.h"

/*===========================================================================*/
/* Mode Functions                                                            */
/*===========================================================================*/

static void external_control_enter(void) {
    DBG_INFO("MODE ExternalCtrl enter: waiting for external commands");
    /* Show a distinctive color to indicate external control mode */
    /* Cyan pulsing indicates waiting for external commands */
    anim_thread_pulse(0, 255, 255, 128, 2000);
}

static void external_control_exit(void) {
    DBG_INFO("MODE ExternalCtrl exit");
    /* Nothing to clean up */
}

static void external_control_on_short_press(void) {
    DBG_DEBUG("MODE ExternalCtrl short_press: ignored (external control active)");
    /* In external control mode, button presses are ignored */
    /* External commands take priority */
}

static void external_control_on_long_start(void) {
    DBG_DEBUG("MODE ExternalCtrl long_start");
    /* No special action for long press start in this mode */
}

/*===========================================================================*/
/* Mode Operations Structure                                                 */
/*===========================================================================*/

const app_sm_mode_ops_t mode_external_control_ops = {
    .name = "External Control",
    .enter = external_control_enter,
    .exit = external_control_exit,
    .on_short_press = external_control_on_short_press,
    .on_long_start = external_control_on_long_start
};
