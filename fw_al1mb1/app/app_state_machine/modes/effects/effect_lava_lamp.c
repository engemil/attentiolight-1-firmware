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
 * @file    effect_lava_lamp.c
 * @brief   Lava lamp effect implementation.
 */

#include "effect_lava_lamp.h"
#include "../../animation/animation_helpers.h"
#include "ch.h"

/*===========================================================================*/
/* Effect Implementation                                                     */
/*===========================================================================*/

void process_lava_lamp(const anim_state_t *state) {
    uint32_t now = chVTGetSystemTime();
    uint32_t elapsed = TIME_I2MS(now - state->start_time);
    uint32_t cycle_pos = elapsed % state->period_ms;
    uint32_t half_period = state->period_ms / 2;

    /* === Primary hue oscillation: triangle wave 0-45-0 (red to orange, no yellow) === */
    uint16_t base_hue;
    if (cycle_pos < half_period) {
        /* Rising: 0 -> 45 (red to orange) */
        base_hue = (uint16_t)((cycle_pos * 45) / half_period);
    } else {
        /* Falling: 45 -> 0 (orange to red) */
        base_hue = (uint16_t)(45 - (((cycle_pos - half_period) * 45) / half_period));
    }
    
    /* === Secondary slower wave: triangle wave ±10 degrees for organic variation === */
    uint32_t slow_period = state->period_ms * 3;
    uint32_t slow_cycle = elapsed % slow_period;
    uint32_t slow_half = slow_period / 2;
    int16_t wave_offset;
    
    if (slow_cycle < slow_half) {
        /* Rising: -10 -> +10 */
        wave_offset = -10 + (int16_t)((slow_cycle * 20) / slow_half);
    } else {
        /* Falling: +10 -> -10 */
        wave_offset = 10 - (int16_t)(((slow_cycle - slow_half) * 20) / slow_half);
    }
    
    /* Combine and clamp hue to 0-45 range (no yellow/green) */
    int16_t hue = (int16_t)base_hue + wave_offset;
    if (hue < 0) hue = 0;
    if (hue > 45) hue = 45;
    
    /* === Saturation: triangle wave 220-255-220 for organic look === */
    uint8_t saturation;
    if (cycle_pos < half_period) {
        saturation = 220 + (uint8_t)((cycle_pos * 35) / half_period);
    } else {
        saturation = 255 - (uint8_t)(((cycle_pos - half_period) * 35) / half_period);
    }
    
    /* === Brightness: triangle wave 85-100-85 for lava blob effect === */
    uint32_t bright_period = state->period_ms * 2 / 3;  /* Slightly different rate */
    uint32_t bright_cycle = elapsed % bright_period;
    uint32_t bright_half = bright_period / 2;
    uint8_t brightness_factor;
    
    if (bright_cycle < bright_half) {
        brightness_factor = 85 + (uint8_t)((bright_cycle * 15) / bright_half);
    } else {
        brightness_factor = 100 - (uint8_t)(((bright_cycle - bright_half) * 15) / bright_half);
    }
    
    uint8_t r, g, b;
    hsv_to_rgb((uint16_t)hue, saturation, 255, &r, &g, &b);
    
    uint8_t effective_brightness = (state->brightness * brightness_factor) / 100;
    render_color(r, g, b, effective_brightness);
}
