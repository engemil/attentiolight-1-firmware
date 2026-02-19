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
 * @file    effect_memory.h
 * @brief   Memory effect configuration and interface.
 *
 * @details Random warm soft glows with irregular timing.
 *          Fast-rise/slow-fade pulses with significant darkness between glows.
 */

#ifndef EFFECT_MEMORY_H
#define EFFECT_MEMORY_H

#include <stdint.h>
#include "../../animation/animation_commands.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================*/
/* Memory Effect Configuration                                               */
/*===========================================================================*/

/**
 * @brief   Memory effect base period in milliseconds.
 * @details Base timing for random warm glows.
 */
#ifndef APP_SM_MEMORY_PERIOD_MS
#define APP_SM_MEMORY_PERIOD_MS         4000
#endif

/*===========================================================================*/
/* Effect Interface                                                          */
/*===========================================================================*/

/**
 * @brief   Processes memory animation tick.
 *
 * @param[in] state     Pointer to current animation state.
 */
void process_memory(const anim_state_t *state);

#ifdef __cplusplus
}
#endif

#endif /* EFFECT_MEMORY_H */
