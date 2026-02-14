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
 * @file    mode_night_light.c
 * @brief   Night light mode implementation.
 *
 * @details Displays a warm, dim light suitable for nighttime use.
 *          Short press cycles through dim brightness levels.
 */

#include "modes.h"
#include "../animation/animation_thread.h"
#include "../app_state_machine_config.h"
#include "app_debug.h"

/*===========================================================================*/
/* Night Light Brightness Levels                                             */
/*===========================================================================*/

#if (APP_DEBUG_LEVEL >= DBG_LEVEL_INFO)
/**
 * @brief   Night light level names for debug output.
 */
static const char* const level_names[4] = {
    "BARELY_VISIBLE", "VERY_DIM", "DIM", "LOW"
};
#endif

/**
 * @brief   Night light brightness levels (very dim).
 */
#define NIGHT_LIGHT_LEVELS  4

static const uint8_t night_brightness_levels[NIGHT_LIGHT_LEVELS] = {
    8,      /* Barely visible */
    16,     /* Very dim */
    32,     /* Dim */
    64      /* Low */
};

/*===========================================================================*/
/* Local Variables                                                           */
/*===========================================================================*/

static uint8_t current_level_index = 2;  /* Default to dim */

/*===========================================================================*/
/* Mode Functions                                                            */
/*===========================================================================*/

static void night_light_enter(void) {
    DBG_INFO("MODE NightLight: enter level=%s (%d)",
             level_names[current_level_index],
             night_brightness_levels[current_level_index]);
    /* Display warm color at current night light brightness */
    anim_thread_set_solid(
        APP_SM_NIGHT_LIGHT_R,
        APP_SM_NIGHT_LIGHT_G,
        APP_SM_NIGHT_LIGHT_B,
        night_brightness_levels[current_level_index]
    );
}

static void night_light_exit(void) {
    DBG_INFO("MODE NightLight: exit");
    /* Nothing to clean up */
}

static void night_light_on_short_press(void) {
    uint8_t old_idx = current_level_index;
    DBG_UNUSED(old_idx);
    /* Cycle to next brightness level */
    current_level_index = (current_level_index + 1) % NIGHT_LIGHT_LEVELS;

    DBG_INFO("MODE NightLight: level set from %s to %s (%d to %d)",
             level_names[old_idx], level_names[current_level_index],
             night_brightness_levels[old_idx],
             night_brightness_levels[current_level_index]);

    /* Update display */
    anim_thread_set_solid(
        APP_SM_NIGHT_LIGHT_R,
        APP_SM_NIGHT_LIGHT_G,
        APP_SM_NIGHT_LIGHT_B,
        night_brightness_levels[current_level_index]
    );
}

static void night_light_on_long_start(void) {
    DBG_DEBUG("MODE NightLight: long_start");
    /* No special action for long press start in this mode */
}

/*===========================================================================*/
/* Mode Operations Structure                                                 */
/*===========================================================================*/

const app_sm_mode_ops_t mode_night_light_ops = {
    .name = "Night Light",
    .enter = night_light_enter,
    .exit = night_light_exit,
    .on_short_press = night_light_on_short_press,
    .on_long_start = night_light_on_long_start
};
