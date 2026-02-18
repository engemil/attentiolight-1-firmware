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
 * @file    mode_effects.c
 * @brief   Effects mode implementation.
 *
 * @details Contains multiple effects sub-modes:
 *          - Rainbow: Smooth HSV color cycling
 *          - Strobe: Fast flash effect
 *          - Color Cycle: Step through colors
 *
 *          Short press cycles through sub-modes.
 */

#include "modes.h"
#include "../animation/animation_thread.h"
#include "../app_state_machine_config.h"
#include "app_debug.h"

/*===========================================================================*/
/* Local Variables                                                           */
/*===========================================================================*/

#if (APP_DEBUG_LEVEL >= DBG_LEVEL_INFO)
/**
 * @brief   Submode names for debug output.
 */
static const char* const submode_names[APP_SM_EFFECTS_COUNT] = {
    "RAINBOW", "STROBE", "COLOR_CYCLE"
};
#endif

static app_sm_effects_submode_t current_submode = APP_SM_EFFECTS_RAINBOW;

/* External reference to global brightness */
extern uint8_t global_brightness;

/*===========================================================================*/
/* Local Functions                                                           */
/*===========================================================================*/

/**
 * @brief   Starts the current effects submode.
 */
static void start_current_submode(void) {
    switch (current_submode) {
        case APP_SM_EFFECTS_RAINBOW:
            anim_thread_rainbow(global_brightness, APP_SM_RAINBOW_PERIOD_MS);
            break;

        case APP_SM_EFFECTS_STROBE:
            anim_thread_strobe(255, 255, 255, global_brightness,
                               APP_SM_STROBE_INTERVAL_MS);
            break;

        case APP_SM_EFFECTS_COLOR_CYCLE:
            anim_thread_color_cycle(global_brightness,
                                    APP_SM_COLOR_CYCLE_INTERVAL_MS);
            break;

        default:
            anim_thread_rainbow(global_brightness, APP_SM_RAINBOW_PERIOD_MS);
            break;
    }
}

/*===========================================================================*/
/* Mode Functions                                                            */
/*===========================================================================*/

static void effects_enter(void) {
    DBG_INFO("MODE Effects: enter submode=%s", submode_names[current_submode]);
    /* Start current effects submode */
    start_current_submode();
}

static void effects_exit(void) {
    DBG_INFO("MODE Effects: exit");
    
}

static void effects_on_short_press(void) {
    app_sm_effects_submode_t old_submode = current_submode;
    DBG_UNUSED(old_submode);
    /* Cycle to next effects submode */
    current_submode = (app_sm_effects_submode_t)
                      ((current_submode + 1) % APP_SM_EFFECTS_COUNT);

    DBG_INFO("MODE Effects: submode %s -> %s",
             submode_names[old_submode], submode_names[current_submode]);

    /* Start new effect */
    start_current_submode();
}

static void effects_on_long_start(void) {
    DBG_DEBUG("MODE Effects: long_start");
    /* No special action for long press start in this mode */
}

/*===========================================================================*/
/* Mode Operations Structure                                                 */
/*===========================================================================*/

const app_sm_mode_ops_t mode_effects_ops = {
    .name = "Effects",
    .enter = effects_enter,
    .exit = effects_exit,
    .on_short_press = effects_on_short_press,
    .on_long_start = effects_on_long_start
};
