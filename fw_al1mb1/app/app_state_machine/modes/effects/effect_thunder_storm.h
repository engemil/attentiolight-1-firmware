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
 * @file    effect_thunder_storm.h
 * @brief   Thunder storm effect configuration and interface.
 *
 * @details Lightning with 2-3 consecutive flashes per strike, variable brightness.
 */

#ifndef EFFECT_THUNDER_STORM_H
#define EFFECT_THUNDER_STORM_H

#include <stdint.h>
#include "../../animation/animation_commands.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================*/
/* Thunder Storm Effect Configuration                                        */
/*===========================================================================*/

/**
 * @brief   Thunder storm base period in milliseconds.
 * @details Average time between lightning flashes.
 */
#ifndef APP_SM_THUNDER_STORM_PERIOD_MS
#define APP_SM_THUNDER_STORM_PERIOD_MS  3000
#endif

/*===========================================================================*/
/* Effect Interface                                                          */
/*===========================================================================*/

/**
 * @brief   Processes thunder storm animation tick.
 *
 * @param[in] state     Pointer to current animation state.
 */
void process_thunder_storm(const anim_state_t *state);

#ifdef __cplusplus
}
#endif

#endif /* EFFECT_THUNDER_STORM_H */
