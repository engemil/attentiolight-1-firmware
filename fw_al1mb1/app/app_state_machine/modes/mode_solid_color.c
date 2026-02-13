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
 * @file    mode_solid_color.c
 * @brief   Solid color mode implementation.
 *
 * @details Displays a static color. Short press cycles through 12 colors.
 */

#include "modes.h"
#include "../animation/animation_thread.h"
#include "../app_state_machine_config.h"
#include "app_debug.h"

/*===========================================================================*/
/* Color Palette                                                             */
/*===========================================================================*/

#if (APP_DEBUG_LEVEL >= DBG_LEVEL_INFO)
/**
 * @brief   Color names for debug output.
 */
static const char* const color_names[APP_SM_COLOR_COUNT] = {
    "RED", "ORANGE", "YELLOW", "LIME", "GREEN", "SPRING",
    "CYAN", "AZURE", "BLUE", "PURPLE", "MAGENTA", "PINK"
};
#endif

/**
 * @brief   12-color extended palette.
 */
static const uint8_t solid_color_palette[APP_SM_COLOR_COUNT][3] = {
    {255,   0,   0},    /* Red          */
    {255, 128,   0},    /* Orange       */
    {255, 255,   0},    /* Yellow       */
    {128, 255,   0},    /* Lime         */
    {  0, 255,   0},    /* Green        */
    {  0, 255, 128},    /* Spring       */
    {  0, 255, 255},    /* Cyan         */
    {  0, 128, 255},    /* Azure        */
    {  0,   0, 255},    /* Blue         */
    {128,   0, 255},    /* Purple       */
    {255,   0, 255},    /* Magenta      */
    {255,   0, 128}     /* Pink         */
};

/*===========================================================================*/
/* Local Variables                                                           */
/*===========================================================================*/

static uint8_t current_color_index = 0;
static uint8_t current_brightness = APP_SM_DEFAULT_BRIGHTNESS;

/*===========================================================================*/
/* Mode Functions                                                            */
/*===========================================================================*/

static void solid_color_enter(void) {
    DBG_INFO("MODE SolidColor enter: color=%s brightness=%d",
             color_names[current_color_index], current_brightness);
    /* Display current color */
    anim_thread_set_solid(
        solid_color_palette[current_color_index][0],
        solid_color_palette[current_color_index][1],
        solid_color_palette[current_color_index][2],
        current_brightness
    );
}

static void solid_color_exit(void) {
    DBG_INFO("MODE SolidColor exit");
    /* Nothing to clean up */
}

static void solid_color_on_short_press(void) {
    uint8_t old_idx = current_color_index;
    DBG_UNUSED(old_idx);
    /* Cycle to next color */
    current_color_index = (current_color_index + 1) % APP_SM_COLOR_COUNT;

    DBG_INFO("MODE SolidColor color %s -> %s",
             color_names[old_idx], color_names[current_color_index]);

    /* Display new color */
    anim_thread_set_solid(
        solid_color_palette[current_color_index][0],
        solid_color_palette[current_color_index][1],
        solid_color_palette[current_color_index][2],
        current_brightness
    );
}

static void solid_color_on_long_start(void) {
    DBG_DEBUG("MODE SolidColor long_start");
    /* No special action for long press start in this mode */
}

/*===========================================================================*/
/* Mode Operations Structure                                                 */
/*===========================================================================*/

const app_sm_mode_ops_t mode_solid_color_ops = {
    .name = "Solid Color",
    .enter = solid_color_enter,
    .exit = solid_color_exit,
    .on_short_press = solid_color_on_short_press,
    .on_long_start = solid_color_on_long_start
};
