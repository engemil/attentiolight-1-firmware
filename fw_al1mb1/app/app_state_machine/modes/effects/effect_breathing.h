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
 * @file    effect_breathing.h
 * @brief   Breathing effect configuration and interface.
 *
 * @details Four-phase breathing: rise -> hold high -> fall -> hold low
 *          Uses configurable timings for each phase.
 */

#ifndef EFFECT_BREATHING_H
#define EFFECT_BREATHING_H

#include <stdint.h>
#include "../../animation/animation_commands.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================*/
/* Breathing Effect Configuration                                            */
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
 * @details This is the sum of all 4 phase durations.
 */
#ifndef APP_SM_BREATHING_PERIOD_MS
#define APP_SM_BREATHING_PERIOD_MS      (APP_SM_BREATHING_RISE_MS + APP_SM_BREATHING_HOLD_HIGH_MS + \
                                         APP_SM_BREATHING_FALL_MS + APP_SM_BREATHING_HOLD_LOW_MS)
#endif

/*===========================================================================*/
/* Effect Interface                                                          */
/*===========================================================================*/

/**
 * @brief   Processes breathing animation tick.
 *
 * @param[in] state     Pointer to current animation state.
 */
void process_breathing(const anim_state_t *state);

#ifdef __cplusplus
}
#endif

#endif /* EFFECT_BREATHING_H */
