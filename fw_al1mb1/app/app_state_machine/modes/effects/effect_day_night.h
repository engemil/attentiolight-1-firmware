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
 * @file    effect_day_night.h
 * @brief   Day/night cycle effect configuration and interface.
 *
 * @details Uses 12 waypoints for ultra-smooth interpolation with sustained phases.
 *          Includes sustained night/day periods and realistic dawn/dusk transitions.
 */

#ifndef EFFECT_DAY_NIGHT_H
#define EFFECT_DAY_NIGHT_H

#include <stdint.h>
#include "animation_commands.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================*/
/* Day/Night Effect Configuration                                            */
/*===========================================================================*/

/**
 * @brief   Day/Night cycle full period in milliseconds.
 * @details Complete sunrise -> day -> sunset -> night cycle.
 */
#ifndef APP_SM_DAY_NIGHT_PERIOD_MS
#define APP_SM_DAY_NIGHT_PERIOD_MS      60000
#endif

/*===========================================================================*/
/* Effect Interface                                                          */
/*===========================================================================*/

/**
 * @brief   Processes day/night cycle animation tick.
 *
 * @param[in] state     Pointer to current animation state.
 */
void process_day_night(const anim_state_t *state);

#ifdef __cplusplus
}
#endif

#endif /* EFFECT_DAY_NIGHT_H */
