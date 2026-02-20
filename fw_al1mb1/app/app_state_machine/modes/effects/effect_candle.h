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
 * @file    effect_candle.h
 * @brief   Candle flicker effect configuration and interface.
 *
 * @details Natural candle: steady gentle drift with occasional subtle flickers
 *          (every 1-5 seconds).
 */

#ifndef EFFECT_CANDLE_H
#define EFFECT_CANDLE_H

#include <stdint.h>
#include "animation_commands.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================*/
/* Candle Effect Configuration                                               */
/*===========================================================================*/

/**
 * @brief   Candle flicker base period in milliseconds.
 * @details Base timing for random flicker variations.
 */
#ifndef APP_SM_CANDLE_PERIOD_MS
#define APP_SM_CANDLE_PERIOD_MS         100
#endif

/**
 * @brief   Candle flicker randomness range in milliseconds.
 * @details Flicker period varies from (base - range/2) to (base + range/2).
 */
#ifndef APP_SM_CANDLE_RANDOM_RANGE_MS
#define APP_SM_CANDLE_RANDOM_RANGE_MS   80
#endif

/*===========================================================================*/
/* Effect Interface                                                          */
/*===========================================================================*/

/**
 * @brief   Processes candle flicker animation tick.
 *
 * @param[in] state     Pointer to current animation state.
 */
void process_candle(const anim_state_t *state);

#ifdef __cplusplus
}
#endif

#endif /* EFFECT_CANDLE_H */
