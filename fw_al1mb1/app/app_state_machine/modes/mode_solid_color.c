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

/*===========================================================================*/
/* Color Palette                                                             */
/*===========================================================================*/

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
    /* Display current color */
    anim_thread_set_solid(
        solid_color_palette[current_color_index][0],
        solid_color_palette[current_color_index][1],
        solid_color_palette[current_color_index][2],
        current_brightness
    );
}

static void solid_color_exit(void) {
    /* Nothing to clean up */
}

static void solid_color_on_short_press(void) {
    /* Cycle to next color */
    current_color_index = (current_color_index + 1) % APP_SM_COLOR_COUNT;

    /* Display new color */
    anim_thread_set_solid(
        solid_color_palette[current_color_index][0],
        solid_color_palette[current_color_index][1],
        solid_color_palette[current_color_index][2],
        current_brightness
    );
}

static void solid_color_on_long_start(void) {
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
