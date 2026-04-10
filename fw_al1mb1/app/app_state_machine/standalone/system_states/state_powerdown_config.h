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
 * @file    state_powerdown_config.h
 * @brief   Configuration defines for the Powerdown state animation.
 */

#ifndef STATE_POWERDOWN_CONFIG_H
#define STATE_POWERDOWN_CONFIG_H

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

#endif /* STATE_POWERDOWN_CONFIG_H */
