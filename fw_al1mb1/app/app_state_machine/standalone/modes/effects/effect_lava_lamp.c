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
#include "animation_helpers.h"

/*===========================================================================*/
/* Effect Implementation                                                     */
/*===========================================================================*/

void process_lava_lamp(const anim_state_t *state) {
    /* Use pre-computed elapsed_ms from animation thread */
    uint32_t elapsed = state->elapsed_ms;
    uint32_t cycle_pos = elapsed % state->period_ms;

    /* === Primary hue oscillation: triangle wave 0-45-0 (red to orange, no yellow) === */
    uint16_t base_hue = (uint16_t)triangle_wave_u8(cycle_pos, state->period_ms, 0, 45);
    
    /* === Secondary slower wave: triangle wave ±10 degrees for organic variation === */
    uint32_t slow_period = state->period_ms * 3;
    uint32_t slow_cycle = elapsed % slow_period;
    int16_t wave_offset;
    
    wave_offset = (int16_t)triangle_wave_u8(slow_cycle, slow_period, 0, 20) - 10;
    
    /* Combine and clamp hue to 0-45 range (no yellow/green) */
    int16_t hue = (int16_t)base_hue + wave_offset;
    if (hue < 0) hue = 0;
    if (hue > 45) hue = 45;
    
    /* === Saturation: triangle wave 220-255-220 for organic look === */
    uint8_t saturation;
    saturation = triangle_wave_u8(cycle_pos, state->period_ms, 220, 255);
    
    /* === Brightness: triangle wave 85-100-85 for lava blob effect === */
    uint32_t bright_period = state->period_ms * 2 / 3;  /* Slightly different rate */
    uint32_t bright_cycle = elapsed % bright_period;
    uint8_t brightness_factor;
    
    brightness_factor = triangle_wave_u8(bright_cycle, bright_period, 85, 100);
    
    uint8_t r, g, b;
    hsv_to_rgb((uint16_t)hue, saturation, 255, &r, &g, &b);
    
    uint8_t effective_brightness = (state->brightness * brightness_factor) / 100;
    render_color(r, g, b, effective_brightness);
}
