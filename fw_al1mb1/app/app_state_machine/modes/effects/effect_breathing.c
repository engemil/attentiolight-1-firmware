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
 * @file    effect_breathing.c
 * @brief   Breathing effect implementation.
 */

#include "effect_breathing.h"
#include "animation_helpers.h"
#include "ch.h"

/*===========================================================================*/
/* Effect Implementation                                                     */
/*===========================================================================*/

void process_breathing(const anim_state_t *state) {
    uint32_t now = chVTGetSystemTime();
    uint32_t elapsed = TIME_I2MS(now - state->start_time);
    
    /* Calculate total cycle from individual phase durations */
    uint32_t total_cycle = APP_SM_BREATHING_RISE_MS + APP_SM_BREATHING_HOLD_HIGH_MS +
                           APP_SM_BREATHING_FALL_MS + APP_SM_BREATHING_HOLD_LOW_MS;
    uint32_t cycle_pos = elapsed % total_cycle;
    
    /* Minimum brightness for hold-low phase (ambient glow) */
    #define BREATHING_MIN_BRIGHTNESS 5
    
    uint8_t brightness_factor;
    
    if (cycle_pos < APP_SM_BREATHING_RISE_MS) {
        /* Rise phase - quadratic ease-in from MIN to 255 */
        uint32_t progress = (cycle_pos * 255) / APP_SM_BREATHING_RISE_MS;
        uint32_t quadratic = (progress * progress) / 255;  /* 0 -> 255 */
        brightness_factor = BREATHING_MIN_BRIGHTNESS + 
                            (uint8_t)((quadratic * (255 - BREATHING_MIN_BRIGHTNESS)) / 255);
    } else if (cycle_pos < APP_SM_BREATHING_RISE_MS + APP_SM_BREATHING_HOLD_HIGH_MS) {
        /* Hold high phase - stay at maximum */
        brightness_factor = 255;
    } else if (cycle_pos < APP_SM_BREATHING_RISE_MS + APP_SM_BREATHING_HOLD_HIGH_MS + 
                           APP_SM_BREATHING_FALL_MS) {
        /* Fall phase - quadratic ease-out from 255 to MIN (smooth continuous decay) */
        uint32_t fall_elapsed = cycle_pos - APP_SM_BREATHING_RISE_MS - APP_SM_BREATHING_HOLD_HIGH_MS;
        uint32_t progress = (fall_elapsed * 255) / APP_SM_BREATHING_FALL_MS;
        /* Inverse quadratic for smooth decay, scaled to land at MIN_BRIGHTNESS */
        uint32_t inv_progress = 255 - progress;
        uint32_t quadratic = (inv_progress * inv_progress) / 255;  /* 255 -> 0 */
        brightness_factor = BREATHING_MIN_BRIGHTNESS + 
                            (uint8_t)((quadratic * (255 - BREATHING_MIN_BRIGHTNESS)) / 255);
    } else {
        /* Hold low phase - stay at minimum */
        brightness_factor = BREATHING_MIN_BRIGHTNESS;
    }
    
    #undef BREATHING_MIN_BRIGHTNESS

    uint8_t effective_brightness = (uint8_t)(((uint16_t)state->brightness * brightness_factor) / 255);
    render_color(state->target_r, state->target_g,
                 state->target_b, effective_brightness);
}
