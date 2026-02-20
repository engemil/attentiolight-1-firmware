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
 * @file    effect_rainbow.c
 * @brief   Rainbow effect implementation.
 */

#include "effect_rainbow.h"
#include "animation_helpers.h"
#include "ch.h"

/*===========================================================================*/
/* Effect Implementation                                                     */
/*===========================================================================*/

void process_rainbow(const anim_state_t *state) {
    systime_t now = chVTGetSystemTime();
    sysinterval_t elapsed_ticks = chTimeDiffX(state->start_time, now);
    uint32_t elapsed = TIME_I2MS(elapsed_ticks);
    uint32_t cycle_pos = elapsed % state->period_ms;

    /* Map cycle position to hue (0-359) */
    uint16_t hue = (uint16_t)((cycle_pos * 360) / state->period_ms);
    uint8_t r, g, b;
    hsv_to_rgb(hue, 255, 255, &r, &g, &b);

    render_color(r, g, b, state->brightness);
}
