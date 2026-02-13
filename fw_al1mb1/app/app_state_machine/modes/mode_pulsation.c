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

/*===========================================================================*/
/* Pulse Speeds                                                              */
/*===========================================================================*/

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
    /* Start pulse animation */
    anim_thread_pulse(pulse_color_r, pulse_color_g, pulse_color_b,
                      global_brightness, pulse_periods[current_speed_index]);
}

static void pulsation_exit(void) {
    /* Nothing to clean up - animation will be replaced */
}

static void pulsation_on_short_press(void) {
    /* Cycle to next pulse speed */
    current_speed_index = (current_speed_index + 1) % PULSE_SPEED_COUNT;

    /* Update pulse animation */
    anim_thread_pulse(pulse_color_r, pulse_color_g, pulse_color_b,
                      global_brightness, pulse_periods[current_speed_index]);
}

static void pulsation_on_long_start(void) {
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
