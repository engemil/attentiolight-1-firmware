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
 */

/* TO DO: Integrate ESP32 WiFi/BLE communication for external control. */
/* TO DO: Implement command protocol for receiving color/animation commands. */
/* TO DO: Add bi-directional communication for status reporting. */
/* TO DO: Register this mode in modes.c when external interfaces are ready. */


#include "modes.h"
#include "animation_thread.h"
#include "standalone_config.h"
#include "app_log.h"

/*===========================================================================*/
/* Mode Functions                                                            */
/*===========================================================================*/

static void external_control_enter(void) {
    LOG_DEBUG("MODE ExternalCtrl: enter waiting for external commands");
    /* TO DO: Initialize ESP32 WiFi/BLE communication here */
    /* TO DO: Register command handlers for external color/animation control */
    /* Show a distinctive color to indicate external control mode */
    /* Cyan pulsing indicates waiting for external commands */
    anim_thread_pulse(0, 255, 255, 128, 2000);
}

static void external_control_exit(void) {
    LOG_DEBUG("MODE ExternalCtrl: exit");
    /* TO DO: Cleanup ESP32 communication and unregister command handlers */
}

static void external_control_on_short_press(void) {
    LOG_DEBUG("MODE ExternalCtrl: short_press ignored (external control active)");
    /* In external control mode, button presses are ignored */
    /* External commands take priority */
}

static void external_control_on_long_start(void) {
    LOG_DEBUG("MODE ExternalCtrl: long_start");
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
