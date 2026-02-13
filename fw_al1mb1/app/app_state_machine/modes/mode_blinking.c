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
 * @file    mode_blinking.c
 * @brief   Blinking mode implementation.
 *
 * @details Blinks LED on/off. Short press cycles through blink speeds.
 */

#include "modes.h"
#include "../animation/animation_thread.h"
#include "../app_state_machine_config.h"

/*===========================================================================*/
/* Blink Speeds                                                              */
/*===========================================================================*/

/**
 * @brief   Blink interval options (ms).
 */
#define BLINK_SPEED_COUNT   5

static const uint16_t blink_speeds[BLINK_SPEED_COUNT] = {
    1000,   /* Very slow - 1Hz */
    500,    /* Slow - 2Hz */
    250,    /* Medium - 4Hz */
    125,    /* Fast - 8Hz */
    62      /* Very fast - ~16Hz */
};

/*===========================================================================*/
/* Local Variables                                                           */
/*===========================================================================*/

static uint8_t current_speed_index = 1;  /* Default to slow */
static uint8_t blink_color_r = 255;
static uint8_t blink_color_g = 255;
static uint8_t blink_color_b = 255;

/* External reference to global brightness */
extern uint8_t global_brightness;

/*===========================================================================*/
/* Mode Functions                                                            */
/*===========================================================================*/

static void blinking_enter(void) {
    /* Start blinking animation */
    anim_thread_blink(blink_color_r, blink_color_g, blink_color_b,
                      global_brightness, blink_speeds[current_speed_index]);
}

static void blinking_exit(void) {
    /* Nothing to clean up - animation will be replaced */
}

static void blinking_on_short_press(void) {
    /* Cycle to next blink speed */
    current_speed_index = (current_speed_index + 1) % BLINK_SPEED_COUNT;

    /* Update blink animation */
    anim_thread_blink(blink_color_r, blink_color_g, blink_color_b,
                      global_brightness, blink_speeds[current_speed_index]);
}

static void blinking_on_long_start(void) {
    /* No special action for long press start in this mode */
}

/*===========================================================================*/
/* Mode Operations Structure                                                 */
/*===========================================================================*/

const app_sm_mode_ops_t mode_blinking_ops = {
    .name = "Blinking",
    .enter = blinking_enter,
    .exit = blinking_exit,
    .on_short_press = blinking_on_short_press,
    .on_long_start = blinking_on_long_start
};
