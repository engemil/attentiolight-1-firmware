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
 * @file    mode_pulsation.c
 * @brief   Pulsation/breathing mode implementation.
 *
 * @details Pulses LED brightness up and down. Short press cycles through
 *          pulse speeds.
 */

#include "modes.h"
#include "../animation/animation_thread.h"
#include "../app_state_machine_config.h"
#include "app_debug.h"

/*===========================================================================*/
/* Pulse Speeds                                                              */
/*===========================================================================*/

#if (APP_DEBUG_LEVEL >= DBG_LEVEL_INFO)
/**
 * @brief   Pulse speed names for debug output.
 */
static const char* const speed_names[5] = {
    "VERY_SLOW", "SLOW", "MEDIUM", "FAST", "VERY_FAST"
};
#endif

/**
 * @brief   Pulse period options (ms for full cycle).
 */
#define PULSE_SPEED_COUNT   5

static const uint16_t pulse_periods[PULSE_SPEED_COUNT] = {
    4000,   /* Very slow - 4s cycle */
    2000,   /* Slow - 2s cycle */
    1000,   /* Medium - 1s cycle */
    500,    /* Fast - 0.5s cycle */
    250     /* Very fast - 0.25s cycle */
};

/*===========================================================================*/
/* Local Variables                                                           */
/*===========================================================================*/

static uint8_t current_speed_index = 1;  /* Default to slow */
static uint8_t pulse_color_r = 255;
static uint8_t pulse_color_g = 255;
static uint8_t pulse_color_b = 255;

/* External reference to global brightness */
extern uint8_t global_brightness;

/*===========================================================================*/
/* Mode Functions                                                            */
/*===========================================================================*/

static void pulsation_enter(void) {
    DBG_INFO("MODE Pulsation enter: speed=%s (%dms period)",
             speed_names[current_speed_index], pulse_periods[current_speed_index]);
    /* Start pulse animation */
    anim_thread_pulse(pulse_color_r, pulse_color_g, pulse_color_b,
                      global_brightness, pulse_periods[current_speed_index]);
}

static void pulsation_exit(void) {
    DBG_INFO("MODE Pulsation exit");
    /* Nothing to clean up - animation will be replaced */
}

static void pulsation_on_short_press(void) {
    uint8_t old_idx = current_speed_index;
    DBG_UNUSED(old_idx);
    /* Cycle to next pulse speed */
    current_speed_index = (current_speed_index + 1) % PULSE_SPEED_COUNT;

    DBG_INFO("MODE Pulsation speed %s -> %s (%dms -> %dms)",
             speed_names[old_idx], speed_names[current_speed_index],
             pulse_periods[old_idx], pulse_periods[current_speed_index]);

    /* Update pulse animation */
    anim_thread_pulse(pulse_color_r, pulse_color_g, pulse_color_b,
                      global_brightness, pulse_periods[current_speed_index]);
}

static void pulsation_on_long_start(void) {
    DBG_DEBUG("MODE Pulsation long_start");
    /* No special action for long press start in this mode */
}

/*===========================================================================*/
/* Mode Operations Structure                                                 */
/*===========================================================================*/

const app_sm_mode_ops_t mode_pulsation_ops = {
    .name = "Pulsation",
    .enter = pulsation_enter,
    .exit = pulsation_exit,
    .on_short_press = pulsation_on_short_press,
    .on_long_start = pulsation_on_long_start
};
