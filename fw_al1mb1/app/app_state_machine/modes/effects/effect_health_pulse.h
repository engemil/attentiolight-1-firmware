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
 * @file    effect_health_pulse.h
 * @brief   Health pulse (heartbeat) effect configuration and interface.
 *
 * @details Red heartbeat double-pulse pattern.
 */

#ifndef EFFECT_HEALTH_PULSE_H
#define EFFECT_HEALTH_PULSE_H

#include <stdint.h>
#include "../../animation/animation_commands.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================*/
/* Health Pulse Effect Configuration                                         */
/*===========================================================================*/

/**
 * @brief   Health pulse (heartbeat) period in milliseconds.
 * @details Time for one complete heartbeat (double-pulse).
 */
#ifndef APP_SM_HEALTH_PULSE_PERIOD_MS
#define APP_SM_HEALTH_PULSE_PERIOD_MS   1600
#endif

/*===========================================================================*/
/* Effect Interface                                                          */
/*===========================================================================*/

/**
 * @brief   Processes health pulse (heartbeat) animation tick.
 *
 * @param[in] state     Pointer to current animation state.
 */
void process_health_pulse(const anim_state_t *state);

#ifdef __cplusplus
}
#endif

#endif /* EFFECT_HEALTH_PULSE_H */
