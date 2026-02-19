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
 * @file    effect_color_cycle.h
 * @brief   Color cycle effect configuration and interface.
 *
 * @details Cycles through a predefined color palette with discrete transitions.
 */

#ifndef EFFECT_COLOR_CYCLE_H
#define EFFECT_COLOR_CYCLE_H

#include <stdint.h>
#include "../../animation/animation_commands.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================*/
/* Color Cycle Effect Configuration                                          */
/*===========================================================================*/

/**
 * @brief   Default interval between color changes in milliseconds.
 */
#ifndef APP_SM_COLOR_CYCLE_INTERVAL_MS
#define APP_SM_COLOR_CYCLE_INTERVAL_MS  1000
#endif

/**
 * @brief   Number of colors in the palette.
 */
#ifndef APP_SM_COLOR_COUNT
#define APP_SM_COLOR_COUNT              12
#endif

/*===========================================================================*/
/* Effect Interface                                                          */
/*===========================================================================*/

/**
 * @brief   Processes color cycle animation tick.
 *
 * @param[in] state             Pointer to current animation state.
 * @param[in,out] last_state    Pointer to last transition state tracker.
 */
void process_color_cycle(const anim_state_t *state, uint8_t *last_state);

#ifdef __cplusplus
}
#endif

#endif /* EFFECT_COLOR_CYCLE_H */
