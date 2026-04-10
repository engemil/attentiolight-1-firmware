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
 * @file    effect_ocean.h
 * @brief   Ocean wave effect configuration and interface.
 *
 * @details Multiple waves (1-3) per cycle with randomized timing and heights.
 *          Wave count: 50% chance of 2 waves, 25% each for 1 or 3 waves.
 */

#ifndef EFFECT_OCEAN_H
#define EFFECT_OCEAN_H

#include <stdint.h>
#include "animation_commands.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================*/
/* Ocean Effect Configuration                                                */
/*===========================================================================*/

/**
 * @brief   Ocean effect wave period in milliseconds.
 */
#ifndef APP_SM_OCEAN_PERIOD_MS
#define APP_SM_OCEAN_PERIOD_MS          5000
#endif

/*===========================================================================*/
/* Effect Interface                                                          */
/*===========================================================================*/

/**
 * @brief   Processes ocean wave animation tick.
 *
 * @param[in] state     Pointer to current animation state.
 */
void process_ocean(const anim_state_t *state);

#ifdef __cplusplus
}
#endif

#endif /* EFFECT_OCEAN_H */
