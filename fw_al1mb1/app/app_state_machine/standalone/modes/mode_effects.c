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
 *          - Color Cycle: Step through colors
 *          - Breathing: Asymmetric breathing (soft azure)
 *          - Candle: Warm flickering candle
 *          - Fire: Intense red/orange flickering
 *          - Lava Lamp: Organic slow color morphing
 *          - Day/Night: Sunrise/sunset cycle
 *          - Ocean: Blue/cyan gentle waves
 *          - Northern Lights: Aurora-like shifting
 *          - Thunder Storm: Dark blue with lightning
 *          - Police: Red/blue smooth transitions
 *          - Health Pulse: Red heartbeat
 *          - Memory: Random warm soft glows
 *
 *          Short press cycles through sub-modes.
 */

#include "modes.h"
#include "animation_thread.h"
#include "standalone_config.h"
#include "app_log.h"

/* Effect configuration includes */
#include "effect_breathing.h"
#include "effect_candle.h"
#include "effect_color_cycle.h"
#include "effect_day_night.h"
#include "effect_fire.h"
#include "effect_health_pulse.h"
#include "effect_lava_lamp.h"
#include "effect_memory.h"
#include "effect_northern_lights.h"
#include "effect_ocean.h"
#include "effect_police.h"
#include "effect_rainbow.h"
#include "effect_thunder_storm.h"

/*===========================================================================*/
/* Local Variables                                                           */
/*===========================================================================*/

/**
 * @brief   Submode names for log output.
 */
static const char* const submode_names[APP_SM_EFFECTS_COUNT] = {
    "RAINBOW", "COLOR_CYCLE", "BREATHING", "CANDLE", "FIRE", "LAVA_LAMP",
    "DAY_NIGHT", "OCEAN", "NORTHERN_LIGHTS", "THUNDER_STORM", "POLICE",
    "HEALTH_PULSE", "MEMORY"
};

static app_sm_effects_submode_t current_submode = APP_SM_EFFECTS_RAINBOW;

/* External reference to global brightness */
//extern uint8_t standalone_brightness;

/*===========================================================================*/
/* Local Functions                                                           */
/*===========================================================================*/

/**
 * @brief   Starts the current effects submode.
 */
static void start_current_submode(void) {
    switch (current_submode) {
        case APP_SM_EFFECTS_RAINBOW:
            anim_thread_rainbow(standalone_brightness, APP_SM_RAINBOW_PERIOD_MS);
            break;

        case APP_SM_EFFECTS_COLOR_CYCLE:
            anim_thread_color_cycle(standalone_brightness,
                                    APP_SM_COLOR_CYCLE_INTERVAL_MS);
            break;

        case APP_SM_EFFECTS_BREATHING:
            anim_thread_breathing(APP_SM_BREATHING_R, APP_SM_BREATHING_G,
                                  APP_SM_BREATHING_B, standalone_brightness,
                                  APP_SM_BREATHING_PERIOD_MS);
            break;

        case APP_SM_EFFECTS_CANDLE:
            anim_thread_candle(standalone_brightness, APP_SM_CANDLE_PERIOD_MS);
            break;

        case APP_SM_EFFECTS_FIRE:
            anim_thread_fire(standalone_brightness, APP_SM_FIRE_PERIOD_MS);
            break;

        case APP_SM_EFFECTS_LAVA_LAMP:
            anim_thread_lava_lamp(standalone_brightness, APP_SM_LAVA_LAMP_PERIOD_MS);
            break;

        case APP_SM_EFFECTS_DAY_NIGHT:
            anim_thread_day_night(standalone_brightness, APP_SM_DAY_NIGHT_PERIOD_MS);
            break;

        case APP_SM_EFFECTS_OCEAN:
            anim_thread_ocean(standalone_brightness, APP_SM_OCEAN_PERIOD_MS);
            break;

        case APP_SM_EFFECTS_NORTHERN_LIGHTS:
            anim_thread_northern_lights(standalone_brightness,
                                        APP_SM_NORTHERN_LIGHTS_PERIOD_MS);
            break;

        case APP_SM_EFFECTS_THUNDER_STORM:
            anim_thread_thunder_storm(standalone_brightness,
                                      APP_SM_THUNDER_STORM_PERIOD_MS);
            break;

        case APP_SM_EFFECTS_POLICE:
            anim_thread_police(standalone_brightness, APP_SM_POLICE_PERIOD_MS);
            break;

        case APP_SM_EFFECTS_HEALTH_PULSE:
            anim_thread_health_pulse(standalone_brightness,
                                     APP_SM_HEALTH_PULSE_PERIOD_MS);
            break;

        case APP_SM_EFFECTS_MEMORY:
            anim_thread_memory(standalone_brightness, APP_SM_MEMORY_PERIOD_MS);
            break;

        default:
            anim_thread_rainbow(standalone_brightness, APP_SM_RAINBOW_PERIOD_MS);
            break;
    }
}

/*===========================================================================*/
/* Mode Functions                                                            */
/*===========================================================================*/

static void effects_enter(void) {
    LOG_DEBUG("MODE Effects: enter submode=%s", submode_names[current_submode]);
    /* Start current effects submode */
    start_current_submode();
}

static void effects_exit(void) {
    LOG_DEBUG("MODE Effects: exit");
    
}

static void effects_on_short_press(void) {
    app_sm_effects_submode_t old_submode = current_submode;
    LOG_UNUSED(old_submode);
    /* Cycle to next effects submode */
    current_submode = (app_sm_effects_submode_t)
                      ((current_submode + 1) % APP_SM_EFFECTS_COUNT);

    LOG_DEBUG("MODE Effects: submode %s -> %s",
             submode_names[old_submode], submode_names[current_submode]);

    /* Start new effect */
    start_current_submode();
}

static void effects_on_long_start(void) {
    LOG_DEBUG("MODE Effects: long_start");
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
