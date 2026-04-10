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
 * @file    mode_brightness.c
 * @brief   Brightness mode implementation.
 *
 * @details Adjusts LED brightness. Short press cycles through 8 brightness
 *          levels. Shows white color to demonstrate brightness.
 */

#include "modes.h"
#include "animation_thread.h"
#include "standalone_config.h"
#include "app_log.h"

/*===========================================================================*/
/* Brightness Levels                                                         */
/*===========================================================================*/

/**
 * @brief   Brightness level names for log output.
 */
static const char* const brightness_names[APP_SM_BRIGHTNESS_LEVELS] = {
    "12%", "25%", "37%", "50%", "62%", "75%", "87%", "100%"
};

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

/* Standalone brightness defined in app_state_machine.c */
//extern uint8_t standalone_brightness;

/*===========================================================================*/
/* Mode Functions                                                            */
/*===========================================================================*/

static void brightness_enter(void) {
    LOG_DEBUG("MODE Brightness: enter level=%s (%d)",
             brightness_names[current_level_index],
             brightness_levels[current_level_index]);
    /* Display current color at current brightness level */
    anim_thread_set_solid(
        standalone_color_palette[standalone_color_index][0],
        standalone_color_palette[standalone_color_index][1],
        standalone_color_palette[standalone_color_index][2],
        brightness_levels[current_level_index]);
}

static void brightness_exit(void) {
    LOG_DEBUG("MODE Brightness: exit standalone_brightness=%d",
             brightness_levels[current_level_index]);
    /* Update standalone brightness for other modes to use */
    standalone_brightness = brightness_levels[current_level_index];
}

static void brightness_on_short_press(void) {
    uint8_t old_idx = current_level_index;
    LOG_UNUSED(old_idx);
    /* Cycle to next brightness level */
    current_level_index = (current_level_index + 1) % APP_SM_BRIGHTNESS_LEVELS;

    LOG_DEBUG("MODE Brightness: level set from %s to %s (%d to %d)",
             brightness_names[old_idx], brightness_names[current_level_index],
             brightness_levels[old_idx], brightness_levels[current_level_index]);

    /* Update display with current color */
    anim_thread_set_solid(
        standalone_color_palette[standalone_color_index][0],
        standalone_color_palette[standalone_color_index][1],
        standalone_color_palette[standalone_color_index][2],
        brightness_levels[current_level_index]);

    /* Update standalone brightness */
    standalone_brightness = brightness_levels[current_level_index];
}

static void brightness_on_long_start(void) {
    LOG_DEBUG("MODE Brightness: long_start");
    /* No special action for long press start in this mode */
}

/*===========================================================================*/
/* Mode Operations Structure                                                 */
/*===========================================================================*/

const app_sm_mode_ops_t mode_brightness_ops = {
    .name = "Brightness",
    .enter = brightness_enter,
    .exit = brightness_exit,
    .on_short_press = brightness_on_short_press,
    .on_long_start = brightness_on_long_start
};
