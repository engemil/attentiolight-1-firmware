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
 * @file    effect_police.c
 * @brief   Police lights effect implementation.
 */

#include "effect_police.h"
#include "animation_helpers.h"
#include "ch.h"

/*===========================================================================*/
/* Effect Implementation                                                     */
/*===========================================================================*/

void process_police(const anim_state_t *state) {
    systime_t now = chVTGetSystemTime();
    sysinterval_t elapsed_ticks = chTimeDiffX(state->start_time, now);
    uint32_t elapsed = TIME_I2MS(elapsed_ticks);
    uint32_t cycle_pos = elapsed % state->period_ms;

    /* Smooth transition: red -> transition -> blue -> transition -> red
     * 0-40%: red with fade
     * 40-50%: red to blue transition
     * 50-90%: blue with fade
     * 90-100%: blue to red transition
     */
    uint32_t phase_pct = (cycle_pos * 100) / state->period_ms;
    
    uint8_t r, g, b;
    
    if (phase_pct < 40) {
        /* Red phase with slight pulse */
        uint8_t pulse = 80 + ((phase_pct * 20) / 40);
        r = 255;
        g = 0;
        b = 0;
        uint8_t effective_brightness = (state->brightness * pulse) / 100;
        render_color(r, g, b, effective_brightness);
    } else if (phase_pct < 50) {
        /* Red to blue transition */
        uint32_t progress = ((phase_pct - 40) * 255) / 10;
        r = 255 - (progress * 255) / 255;
        g = 0;
        b = (progress * 255) / 255;
        render_color(r, g, b, state->brightness);
    } else if (phase_pct < 90) {
        /* Blue phase with slight pulse */
        uint8_t pulse = 80 + (((phase_pct - 50) * 20) / 40);
        r = 0;
        g = 0;
        b = 255;
        uint8_t effective_brightness = (state->brightness * pulse) / 100;
        render_color(r, g, b, effective_brightness);
    } else {
        /* Blue to red transition */
        uint32_t progress = ((phase_pct - 90) * 255) / 10;
        r = (progress * 255) / 255;
        g = 0;
        b = 255 - (progress * 255) / 255;
        render_color(r, g, b, state->brightness);
    }
}
