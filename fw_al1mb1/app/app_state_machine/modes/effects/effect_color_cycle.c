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
 * @file    effect_color_cycle.c
 * @brief   Color cycle effect implementation.
 */

#include "effect_color_cycle.h"
#include "animation_helpers.h"
#include "modes.h"

/*===========================================================================*/
/* Effect Implementation                                                     */
/*===========================================================================*/

void process_color_cycle(const anim_state_t *state, uint8_t *last_state) {
    /* Use pre-computed elapsed_ms from animation thread */
    uint32_t total_period = state->period_ms * APP_SM_COLOR_COUNT;
    uint32_t cycle_pos = state->elapsed_ms % total_period;

    /* Determine current color index */
    uint8_t color_idx = (uint8_t)(cycle_pos / state->period_ms);
    if (color_idx >= APP_SM_COLOR_COUNT) {
        color_idx = 0;
    }

    /* Only render on color transition */
    if (color_idx != *last_state) {
        *last_state = color_idx;
        render_color(shared_color_palette[color_idx][0],
                     shared_color_palette[color_idx][1],
                     shared_color_palette[color_idx][2],
                     state->brightness);
    }
}
