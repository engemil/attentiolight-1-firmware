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
 * @file    state_powerup_config.h
 * @brief   Configuration defines for the Powerup state animation.
 */

#ifndef STATE_POWERUP_CONFIG_H
#define STATE_POWERUP_CONFIG_H

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

#endif /* STATE_POWERUP_CONFIG_H */
