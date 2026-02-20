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
 * @file    effect_fire.h
 * @brief   Fire effect configuration and interface.
 *
 * @details Natural fire: glowing base with occasional flicker bursts
 *          (every 0.5-2 seconds). Red/orange dominant with occasional
 *          yellow highlights, no greenish tint.
 */

#ifndef EFFECT_FIRE_H
#define EFFECT_FIRE_H

#include <stdint.h>
#include "animation_commands.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================*/
/* Fire Effect Configuration                                                 */
/*===========================================================================*/

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

/*===========================================================================*/
/* Effect Interface                                                          */
/*===========================================================================*/

/**
 * @brief   Processes fire animation tick.
 *
 * @param[in] state     Pointer to current animation state.
 */
void process_fire(const anim_state_t *state);

#ifdef __cplusplus
}
#endif

#endif /* EFFECT_FIRE_H */
