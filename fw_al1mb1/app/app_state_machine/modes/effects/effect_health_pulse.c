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
 * @file    effect_health_pulse.c
 * @brief   Health pulse (heartbeat) effect implementation.
 */

#include "effect_health_pulse.h"
#include "animation_helpers.h"

/*===========================================================================*/
/* Effect Implementation                                                     */
/*===========================================================================*/

void process_health_pulse(const anim_state_t *state) {
    /* Use pre-computed elapsed_ms from animation thread */
    uint32_t cycle_pos = state->elapsed_ms % state->period_ms;

    /* Heartbeat pattern: quick beat, pause, quick beat, long pause
     * 0-15%:  First beat (up and down)
     * 15-30%: Short pause
     * 30-45%: Second beat (up and down)
     * 45-100%: Long pause
     */
    uint32_t phase_pct = (cycle_pos * 100) / state->period_ms;
    
    uint8_t brightness_factor;
    uint8_t current_state;
    
    if (phase_pct < 8) {
        /* First beat rise */
        brightness_factor = (phase_pct * 100) / 8;
        current_state = 1;
    } else if (phase_pct < 15) {
        /* First beat fall */
        brightness_factor = 100 - (((phase_pct - 8) * 100) / 7);
        current_state = 2;
    } else if (phase_pct < 30) {
        /* Short pause */
        brightness_factor = 10;
        current_state = 3;
    } else if (phase_pct < 38) {
        /* Second beat rise */
        brightness_factor = 10 + (((phase_pct - 30) * 70) / 8);  /* Slightly weaker */
        current_state = 4;
    } else if (phase_pct < 45) {
        /* Second beat fall */
        brightness_factor = 80 - (((phase_pct - 38) * 70) / 7);
        current_state = 5;
    } else {
        /* Long pause */
        brightness_factor = 10;
        current_state = 6;
    }
    
    /* Only render on state transition for efficiency */
    if (current_state != last_transition_state || 
        current_state == 1 || current_state == 2 || 
        current_state == 4 || current_state == 5) {
        last_transition_state = current_state;
        uint8_t effective_brightness = (state->brightness * brightness_factor) / 100;
        /* Red heartbeat color */
        render_color(255, 0, 30, effective_brightness);
    }
}
