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
 * @file    effect_northern_lights.h
 * @brief   Northern lights (aurora) effect configuration and interface.
 *
 * @details Aurora episodes with dark periods in between. Super-cycle is 3x period.
 *          Dark (dim) -> Fade in -> Active aurora -> Fade out -> Dark
 */

#ifndef EFFECT_NORTHERN_LIGHTS_H
#define EFFECT_NORTHERN_LIGHTS_H

#include <stdint.h>
#include "../../animation/animation_commands.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================*/
/* Northern Lights Effect Configuration                                      */
/*===========================================================================*/

/**
 * @brief   Northern lights effect period in milliseconds.
 */
#ifndef APP_SM_NORTHERN_LIGHTS_PERIOD_MS
#define APP_SM_NORTHERN_LIGHTS_PERIOD_MS 10000
#endif

/*===========================================================================*/
/* Effect Interface                                                          */
/*===========================================================================*/

/**
 * @brief   Processes northern lights (aurora) animation tick.
 *
 * @param[in] state     Pointer to current animation state.
 */
void process_northern_lights(const anim_state_t *state);

#ifdef __cplusplus
}
#endif

#endif /* EFFECT_NORTHERN_LIGHTS_H */
