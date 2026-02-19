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
 * @file    app_state_machine_config.h
 * @brief   Configuration defines for the Application State Machine.
 */

#ifndef APP_STATE_MACHINE_CONFIG_H
#define APP_STATE_MACHINE_CONFIG_H

/*===========================================================================*/
/* Thread Configuration                                                      */
/*===========================================================================*/

/**
 * @brief   State machine thread working area size.
 */
#ifndef APP_SM_THREAD_WA_SIZE
#define APP_SM_THREAD_WA_SIZE           512
#endif

/**
 * @brief   State machine thread priority.
 */
#ifndef APP_SM_THREAD_PRIORITY
#define APP_SM_THREAD_PRIORITY          (NORMALPRIO)
#endif

/**
 * @brief   Animation thread working area size.
 */
#ifndef APP_SM_ANIM_THREAD_WA_SIZE
#define APP_SM_ANIM_THREAD_WA_SIZE      512
#endif

/**
 * @brief   Animation thread priority.
 * @details Higher than SM thread for smooth animations.
 */
#ifndef APP_SM_ANIM_THREAD_PRIORITY
#define APP_SM_ANIM_THREAD_PRIORITY     (NORMALPRIO + 1)
#endif

/**
 * @brief   Input event queue size.
 */
#ifndef APP_SM_INPUT_QUEUE_SIZE
#define APP_SM_INPUT_QUEUE_SIZE         8
#endif

/*===========================================================================*/
/* Timing Configuration                                                      */
/*===========================================================================*/

/**
 * @brief   Powerup animation duration in milliseconds.
 */
#ifndef APP_SM_POWERUP_FADE_MS
#define APP_SM_POWERUP_FADE_MS          1000
#endif

/**
 * @brief   Powerdown animation duration in milliseconds.
 */
#ifndef APP_SM_POWERDOWN_FADE_MS
#define APP_SM_POWERDOWN_FADE_MS        500
#endif

/**
 * @brief   Animation update interval in milliseconds.
 * @details 33ms = ~30 FPS, good balance of smoothness and CPU usage.
 *          Reduced from 20ms (50 FPS) to improve timing stability.
 */
#ifndef APP_SM_ANIM_TICK_MS
#define APP_SM_ANIM_TICK_MS             33
#endif

/**
 * @brief   Mode transition feedback duration in milliseconds.
 */
#ifndef APP_SM_MODE_FEEDBACK_MS
#define APP_SM_MODE_FEEDBACK_MS         100
#endif

/**
 * @brief   Brightness/intensity of the mode change feedback flash.
 * @details Value from 0 (off) to 255 (maximum brightness).
 */
#ifndef APP_SM_CHANGE_MODE_FEEDBACK_BRIGHTNESS
#define APP_SM_CHANGE_MODE_FEEDBACK_BRIGHTNESS  128
#endif


/*===========================================================================*/
/* Feature Toggles                                                           */
/*===========================================================================*/

/**
 * @brief   Enable visual feedback on long press start.
 * @details Set to 1 to show feedback when long press threshold is reached.
 *          Set to 0 to disable feedback until release.
 */
#ifndef APP_SM_LONG_PRESS_FEEDBACK
#define APP_SM_LONG_PRESS_FEEDBACK      1
#endif

/**
 * @brief   Enable mode persistence to flash.
 * @details Set to 1 to save current mode to flash on powerdown.
 *          Requires flash driver implementation.
 */
#ifndef APP_SM_PERSIST_MODE
#define APP_SM_PERSIST_MODE             0
#endif

/**
 * @brief   Enable debug output.
 */
#ifndef APP_SM_DEBUG
#define APP_SM_DEBUG                    0
#endif

/*===========================================================================*/
/* Default Values                                                            */
/*===========================================================================*/

/**
 * @brief   Default operational mode.
 */
#ifndef APP_SM_DEFAULT_MODE
#define APP_SM_DEFAULT_MODE             APP_SM_MODE_SOLID_COLOR
#endif

/**
 * @brief   Default brightness level (0-255).
 */
#ifndef APP_SM_DEFAULT_BRIGHTNESS
#define APP_SM_DEFAULT_BRIGHTNESS       128
#endif

/**
 * @brief   Default color index for solid color mode.
 * 
 * @details Index into shared_color_palette (see mode_solid_color.c).
 *          Set to 0 for first color, or any
 */
#ifndef APP_SM_DEFAULT_COLOR_INDEX
#define APP_SM_DEFAULT_COLOR_INDEX      0
#endif

/*===========================================================================*/
/* Color Palette Configuration                                               */
/*===========================================================================*/

/**
 * @brief   Number of colors in the solid color palette.
 */
#define APP_SM_COLOR_COUNT              12

/**
 * @brief   Number of brightness levels in brightness mode.
 */
#define APP_SM_BRIGHTNESS_LEVELS        8

/*===========================================================================*/
/* Animation Configuration                                                   */
/*===========================================================================*/

/**
 * @brief   Default blink interval in milliseconds.
 */
#ifndef APP_SM_BLINK_INTERVAL_MS
#define APP_SM_BLINK_INTERVAL_MS        500
#endif

/**
 * @brief   Default pulse period in milliseconds.
 */
#ifndef APP_SM_PULSE_PERIOD_MS
#define APP_SM_PULSE_PERIOD_MS          2000
#endif

/**
 * @brief   Default rainbow cycle period in milliseconds.
 */
#ifndef APP_SM_RAINBOW_PERIOD_MS
#define APP_SM_RAINBOW_PERIOD_MS        5000
#endif

/**
 * @brief   Default color cycle interval in milliseconds.
 */
#ifndef APP_SM_COLOR_CYCLE_INTERVAL_MS
#define APP_SM_COLOR_CYCLE_INTERVAL_MS  1000
#endif

/*===========================================================================*/
/* Effects Mode Animation Configuration                                      */
/*===========================================================================*/

/**
 * @brief   Breathing effect: rise time in milliseconds.
 * @details Time to go from low to high brightness.
 */
#ifndef APP_SM_BREATHING_RISE_MS
#define APP_SM_BREATHING_RISE_MS        3000
#endif

/**
 * @brief   Breathing effect: fall time in milliseconds.
 * @details Time to go from high to low brightness.
 */
#ifndef APP_SM_BREATHING_FALL_MS
#define APP_SM_BREATHING_FALL_MS        4000
#endif

/**
 * @brief   Breathing effect: hold high time in milliseconds.
 * @details Time to stay at peak brightness.
 */
#ifndef APP_SM_BREATHING_HOLD_HIGH_MS
#define APP_SM_BREATHING_HOLD_HIGH_MS   2000
#endif

/**
 * @brief   Breathing effect: hold low time in milliseconds.
 * @details Time to stay at minimum brightness.
 */
#ifndef APP_SM_BREATHING_HOLD_LOW_MS
#define APP_SM_BREATHING_HOLD_LOW_MS    2000
#endif

/**
 * @brief   Breathing effect color (soft azure).
 */
#ifndef APP_SM_BREATHING_R
#define APP_SM_BREATHING_R              180
#endif
#ifndef APP_SM_BREATHING_G
#define APP_SM_BREATHING_G              220
#endif
#ifndef APP_SM_BREATHING_B
#define APP_SM_BREATHING_B              255
#endif

/**
 * @brief   Breathing effect total period (computed from phases).
 * @details This is the sum of all 4 phase durations. Used by mode_effects.c.
 */
#ifndef APP_SM_BREATHING_PERIOD_MS
#define APP_SM_BREATHING_PERIOD_MS      (APP_SM_BREATHING_RISE_MS + APP_SM_BREATHING_HOLD_HIGH_MS + \
                                         APP_SM_BREATHING_FALL_MS + APP_SM_BREATHING_HOLD_LOW_MS)
#endif

/**
 * @brief   Candle flicker base period in milliseconds.
 * @details Base timing for random flicker variations.
 */
#ifndef APP_SM_CANDLE_PERIOD_MS
#define APP_SM_CANDLE_PERIOD_MS         100
#endif

/**
 * @brief   Candle flicker randomness range in milliseconds.
 * @details Flicker period varies from (base - range/2) to (base + range/2).
 */
#ifndef APP_SM_CANDLE_RANDOM_RANGE_MS
#define APP_SM_CANDLE_RANDOM_RANGE_MS   80
#endif

/**
 * @brief   Fire effect base period in milliseconds.
 */
#ifndef APP_SM_FIRE_PERIOD_MS
#define APP_SM_FIRE_PERIOD_MS           60
#endif

/**
 * @brief   Fire effect randomness range in milliseconds.
 * @details Flicker period varies randomly within this range.
 */
#ifndef APP_SM_FIRE_RANDOM_RANGE_MS
#define APP_SM_FIRE_RANDOM_RANGE_MS     100
#endif

/**
 * @brief   Lava lamp effect period in milliseconds.
 * @details Full color morph cycle.
 */
#ifndef APP_SM_LAVA_LAMP_PERIOD_MS
#define APP_SM_LAVA_LAMP_PERIOD_MS      8000
#endif

/**
 * @brief   Day/Night cycle full period in milliseconds.
 * @details Complete sunrise -> day -> sunset -> night cycle.
 */
#ifndef APP_SM_DAY_NIGHT_PERIOD_MS
#define APP_SM_DAY_NIGHT_PERIOD_MS      60000
#endif

/**
 * @brief   Ocean effect wave period in milliseconds.
 */
#ifndef APP_SM_OCEAN_PERIOD_MS
#define APP_SM_OCEAN_PERIOD_MS          5000
#endif

/**
 * @brief   Northern lights effect period in milliseconds.
 */
#ifndef APP_SM_NORTHERN_LIGHTS_PERIOD_MS
#define APP_SM_NORTHERN_LIGHTS_PERIOD_MS 10000
#endif

/**
 * @brief   Thunder storm base period in milliseconds.
 * @details Average time between lightning flashes.
 */
#ifndef APP_SM_THUNDER_STORM_PERIOD_MS
#define APP_SM_THUNDER_STORM_PERIOD_MS  3000
#endif

/**
 * @brief   Police lights transition period in milliseconds.
 * @details Time for one red-blue cycle with smooth transitions.
 */
#ifndef APP_SM_POLICE_PERIOD_MS
#define APP_SM_POLICE_PERIOD_MS         800
#endif

/**
 * @brief   Health pulse (heartbeat) period in milliseconds.
 * @details Time for one complete heartbeat (double-pulse).
 */
#ifndef APP_SM_HEALTH_PULSE_PERIOD_MS
#define APP_SM_HEALTH_PULSE_PERIOD_MS   1600
#endif

/**
 * @brief   Memory effect base period in milliseconds.
 * @details Base timing for random warm glows.
 */
#ifndef APP_SM_MEMORY_PERIOD_MS
#define APP_SM_MEMORY_PERIOD_MS         4000
#endif

/**
 * @brief   Traffic light red duration in milliseconds.
 */
#ifndef APP_SM_TRAFFIC_RED_MS
#define APP_SM_TRAFFIC_RED_MS           3000
#endif

/**
 * @brief   Traffic light yellow duration in milliseconds.
 */
#ifndef APP_SM_TRAFFIC_YELLOW_MS
#define APP_SM_TRAFFIC_YELLOW_MS        1000
#endif

/**
 * @brief   Traffic light green duration in milliseconds.
 */
#ifndef APP_SM_TRAFFIC_GREEN_MS
#define APP_SM_TRAFFIC_GREEN_MS         3000
#endif

/**
 * @brief   Night light warm color (amber/warm white).
 */
#ifndef APP_SM_NIGHT_LIGHT_R
#define APP_SM_NIGHT_LIGHT_R            255
#endif
#ifndef APP_SM_NIGHT_LIGHT_G
#define APP_SM_NIGHT_LIGHT_G            147
#endif
#ifndef APP_SM_NIGHT_LIGHT_B
#define APP_SM_NIGHT_LIGHT_B            41
#endif

/**
 * @brief   Night light default brightness (lower for night use).
 */
#ifndef APP_SM_NIGHT_LIGHT_BRIGHTNESS
#define APP_SM_NIGHT_LIGHT_BRIGHTNESS   32
#endif

/*===========================================================================*/
/* Powerup Animation Configuration                                           */
/*===========================================================================*/

/**
 * @brief   Powerup animation: rainbow fade-in duration in milliseconds.
 * @details Phase 1 - LED fades in while cycling through colors.
 */
#ifndef APP_SM_POWERUP_RAINBOW_FADE_MS
#define APP_SM_POWERUP_RAINBOW_FADE_MS      4000
#endif

/**
 * @brief   Powerup animation: rainbow cycle period at START in milliseconds.
 * @details Initial (slow) color cycle speed when fade begins.
 *          Higher values = slower color changes at the start.
 */
#ifndef APP_SM_POWERUP_RAINBOW_CYCLE_START_MS
#define APP_SM_POWERUP_RAINBOW_CYCLE_START_MS   4000
#endif

/**
 * @brief   Powerup animation: rainbow cycle period at END in milliseconds.
 * @details Final (fast) color cycle speed when fade completes.
 *          Lower values = faster color changes at the end.
 *          Set equal to START_MS to disable acceleration.
 */
#ifndef APP_SM_POWERUP_RAINBOW_CYCLE_END_MS
#define APP_SM_POWERUP_RAINBOW_CYCLE_END_MS     250
#endif

/**
 * @brief   Powerup animation: transition to blue duration in milliseconds.
 * @details Phase 2 - Fades from current rainbow color to solid blue.
 */
#ifndef APP_SM_POWERUP_BLUE_FADE_MS
#define APP_SM_POWERUP_BLUE_FADE_MS         500
#endif

/**
 * @brief   Powerup animation: pulse period in milliseconds.
 * @details Phase 3 - Duration of one complete pulse cycle (up and down).
 */
#ifndef APP_SM_POWERUP_PULSE_PERIOD_MS
#define APP_SM_POWERUP_PULSE_PERIOD_MS      1500
#endif

/**
 * @brief   Powerup animation: number of blue pulses.
 * @details Phase 3 - How many times to pulse before going blank.
 */
#ifndef APP_SM_POWERUP_PULSE_COUNT
#define APP_SM_POWERUP_PULSE_COUNT          2
#endif

/**
 * @brief   Powerup animation: blank duration in milliseconds.
 * @details Phase 4 - LED stays off before transitioning to active state.
 */
#ifndef APP_SM_POWERUP_BLANK_MS
#define APP_SM_POWERUP_BLANK_MS             800
#endif

/**
 * @brief   Powerup animation: final color (blue).
 * @details RGB values for the final blue color in the powerup sequence.
 */
#ifndef APP_SM_POWERUP_BLUE_R
#define APP_SM_POWERUP_BLUE_R               0
#endif
#ifndef APP_SM_POWERUP_BLUE_G
#define APP_SM_POWERUP_BLUE_G               0
#endif
#ifndef APP_SM_POWERUP_BLUE_B
#define APP_SM_POWERUP_BLUE_B               255
#endif

/**
 * @brief   Total powerup animation duration in milliseconds.
 * @details Calculated from all phases. Used for powerup timer.
 */
#define APP_SM_POWERUP_TOTAL_MS             (APP_SM_POWERUP_RAINBOW_FADE_MS + \
                                             APP_SM_POWERUP_BLUE_FADE_MS + \
                                             (APP_SM_POWERUP_PULSE_PERIOD_MS * APP_SM_POWERUP_PULSE_COUNT) + \
                                             APP_SM_POWERUP_BLANK_MS)

/*===========================================================================*/
/* Powerdown Animation Configuration                                         */
/*===========================================================================*/

/**
 * @brief   Powerdown animation: color transition duration in milliseconds.
 * @details Phase 0 - Fades from current color to amber.
 */
#ifndef APP_SM_POWERDOWN_COLOR_FADE_MS
#define APP_SM_POWERDOWN_COLOR_FADE_MS       200
#endif

/**
 * @brief   Powerdown animation: pulse period in milliseconds.
 * @details Duration of one complete pulse cycle (dim down and back up).
 */
#ifndef APP_SM_POWERDOWN_PULSE_PERIOD_MS
#define APP_SM_POWERDOWN_PULSE_PERIOD_MS     1000
#endif

/**
 * @brief   Powerdown animation: number of pulses.
 * @details How many breathing cycles before final fade.
 */
#ifndef APP_SM_POWERDOWN_PULSE_COUNT
#define APP_SM_POWERDOWN_PULSE_COUNT         3
#endif

/**
 * @brief   Powerdown animation: final fade duration in milliseconds.
 * @details Final smooth fade from dim to completely off.
 */
#ifndef APP_SM_POWERDOWN_FINAL_FADE_MS
#define APP_SM_POWERDOWN_FINAL_FADE_MS       1000
#endif

/**
 * @brief   Powerdown animation: amber color (warm).
 * @details RGB values for the amber color used in powerdown sequence.
 */
#ifndef APP_SM_POWERDOWN_AMBER_R
#define APP_SM_POWERDOWN_AMBER_R             255
#endif
#ifndef APP_SM_POWERDOWN_AMBER_G
#define APP_SM_POWERDOWN_AMBER_G             147
#endif
#ifndef APP_SM_POWERDOWN_AMBER_B
#define APP_SM_POWERDOWN_AMBER_B             41
#endif

/**
 * @brief   Total powerdown animation duration in milliseconds.
 * @details Calculated from all phases. Used for powerdown timer.
 */
#define APP_SM_POWERDOWN_TOTAL_MS            (APP_SM_POWERDOWN_COLOR_FADE_MS + \
                                             (APP_SM_POWERDOWN_PULSE_PERIOD_MS * APP_SM_POWERDOWN_PULSE_COUNT) + \
                                             APP_SM_POWERDOWN_FINAL_FADE_MS)

#endif /* APP_STATE_MACHINE_CONFIG_H */
