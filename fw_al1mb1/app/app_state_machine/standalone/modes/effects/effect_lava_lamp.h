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
 * @file    effect_lava_lamp.h
 * @brief   Lava lamp effect configuration and interface.
 *
 * @details Organic slow color morphing. Restricted to warm tones:
 *          red/orange only (hue 0-45), no yellow/green.
 *          All transitions use triangle waves to avoid discontinuities.
 */

#ifndef EFFECT_LAVA_LAMP_H
#define EFFECT_LAVA_LAMP_H

#include <stdint.h>
#include "animation_commands.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================*/
/* Lava Lamp Effect Configuration                                            */
/*===========================================================================*/

/**
 * @brief   Lava lamp effect period in milliseconds.
 * @details Full color morph cycle.
 */
#ifndef APP_SM_LAVA_LAMP_PERIOD_MS
#define APP_SM_LAVA_LAMP_PERIOD_MS      8000
#endif

/*===========================================================================*/
/* Effect Interface                                                          */
/*===========================================================================*/

/**
 * @brief   Processes lava lamp animation tick.
 *
 * @param[in] state     Pointer to current animation state.
 */
void process_lava_lamp(const anim_state_t *state);

#ifdef __cplusplus
}
#endif

#endif /* EFFECT_LAVA_LAMP_H */
