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
#include "app_debug.h"

/*===========================================================================*/
/* Blink Speeds                                                              */
/*===========================================================================*/

#if (APP_DEBUG_LEVEL >= DBG_LEVEL_DEBUG)
/**
 * @brief   Blink speed names for debug output.
 */
static const char* const speed_names[6] = {
    "ULTRA_SLOW", "VERY_SLOW", "SLOW", "MEDIUM", "FAST", "VERY_FAST"
};
#endif

/**
 * @brief   Blink interval options (ms).
 */
#define BLINK_SPEED_COUNT   5

static const uint16_t blink_speeds[BLINK_SPEED_COUNT] = {
    4000,   /* Ultra Slow - 0.25Hz */
    2000,   /* Very Slow - 0.5Hz */
    1000,   /* Slow - 1Hz */
    500,    /* Medium - 2Hz */
    250,    /* Fast - 4Hz */
    125     /* Very Fast - 8Hz */
};

/*===========================================================================*/
/* Local Variables                                                           */
/*===========================================================================*/

static uint8_t current_speed_index = 2;  /* Default to Slow */

/* Color and brightness are shared via global_color_index, global_brightness,
 * and shared_color_palette (declared in modes.h) */

/*===========================================================================*/
/* Mode Functions                                                            */
/*===========================================================================*/

static void blinking_enter(void) {
    DBG_DEBUG("MODE Blinking: enter speed=%s (%dms)",
             speed_names[current_speed_index], blink_speeds[current_speed_index]);
    /* Start blinking animation with shared color */
    anim_thread_blink(
        shared_color_palette[global_color_index][0],
        shared_color_palette[global_color_index][1],
        shared_color_palette[global_color_index][2],
        global_brightness, blink_speeds[current_speed_index]);
}

static void blinking_exit(void) {
    DBG_DEBUG("MODE Blinking: exit");
    
}

static void blinking_on_short_press(void) {
    uint8_t old_idx = current_speed_index;
    DBG_UNUSED(old_idx);
    /* Cycle to next blink speed */
    current_speed_index = (current_speed_index + 1) % BLINK_SPEED_COUNT;

    DBG_DEBUG("MODE Blinking: speed set from %s to %s (%dms to %dms)",
             speed_names[old_idx], speed_names[current_speed_index],
             blink_speeds[old_idx], blink_speeds[current_speed_index]);

    /* Update blink animation with shared color */
    anim_thread_blink(
        shared_color_palette[global_color_index][0],
        shared_color_palette[global_color_index][1],
        shared_color_palette[global_color_index][2],
        global_brightness, blink_speeds[current_speed_index]);
}

static void blinking_on_long_start(void) {
    DBG_DEBUG("MODE Blinking: long_start");
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
