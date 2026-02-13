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
 * @file    mode_strength.c
 * @brief   Strength/brightness mode implementation.
 *
 * @details Adjusts LED brightness. Short press cycles through 8 brightness
 *          levels. Shows white color to demonstrate brightness.
 */

#include "modes.h"
#include "../animation/animation_thread.h"
#include "../app_state_machine_config.h"

/*===========================================================================*/
/* Brightness Levels                                                         */
/*===========================================================================*/

/**
 * @brief   8 brightness levels (12% to 100%).
 */
static const uint8_t brightness_levels[APP_SM_BRIGHTNESS_LEVELS] = {
    32,     /* 12% */
    64,     /* 25% */
    96,     /* 37% */
    128,    /* 50% */
    160,    /* 62% */
    192,    /* 75% */
    224,    /* 87% */
    255     /* 100% */
};

/*===========================================================================*/
/* Local Variables                                                           */
/*===========================================================================*/

static uint8_t current_level_index = 3;  /* Default to 50% */

/* Global brightness defined in app_state_machine.c */
extern uint8_t global_brightness;

/*===========================================================================*/
/* Mode Functions                                                            */
/*===========================================================================*/

static void strength_enter(void) {
    /* Display white at current brightness level */
    anim_thread_set_solid(255, 255, 255, brightness_levels[current_level_index]);
}

static void strength_exit(void) {
    /* Update global brightness for other modes to use */
    global_brightness = brightness_levels[current_level_index];
}

static void strength_on_short_press(void) {
    /* Cycle to next brightness level */
    current_level_index = (current_level_index + 1) % APP_SM_BRIGHTNESS_LEVELS;

    /* Update display */
    anim_thread_set_solid(255, 255, 255, brightness_levels[current_level_index]);

    /* Update global brightness */
    global_brightness = brightness_levels[current_level_index];
}

static void strength_on_long_start(void) {
    /* No special action for long press start in this mode */
}

/*===========================================================================*/
/* Mode Operations Structure                                                 */
/*===========================================================================*/

const app_sm_mode_ops_t mode_strength_ops = {
    .name = "Strength",
    .enter = strength_enter,
    .exit = strength_exit,
    .on_short_press = strength_on_short_press,
    .on_long_start = strength_on_long_start
};
