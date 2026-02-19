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
 * @file    effect_ocean.c
 * @brief   Ocean wave effect implementation.
 */

#include "effect_ocean.h"
#include "../../animation/animation_helpers.h"
#include "ch.h"

/*===========================================================================*/
/* Effect Implementation                                                     */
/*===========================================================================*/

void process_ocean(const anim_state_t *state) {
    uint32_t now = chVTGetSystemTime();
    uint32_t elapsed = TIME_I2MS(now - state->start_time);
    
    /* Determine which cycle we're in */
    uint32_t cycle_num = elapsed / state->period_ms;
    uint32_t cycle_pos = elapsed % state->period_ms;
    
    /* Seed random based on cycle number for consistent waves within each cycle */
    effect_random_seed = cycle_num * 7919 + 13;
    uint32_t rand_count = effect_random();
    uint32_t rand_timing = effect_random();
    uint32_t rand_heights = effect_random();
    
    /* Determine wave count: 25% for 1, 50% for 2, 25% for 3 */
    uint8_t wave_count;
    uint8_t count_roll = rand_count % 100;
    if (count_roll < 25) {
        wave_count = 1;
    } else if (count_roll < 75) {
        wave_count = 2;
    } else {
        wave_count = 3;
    }
    
    /* Calculate base wave period and randomized offsets */
    uint32_t base_wave_period = state->period_ms / wave_count;
    
    /* Randomize wave timing: each wave can shift ±15% of its slot */
    /* Build wave boundaries with random offsets */
    uint32_t wave_starts[4];  /* Start positions for up to 3 waves + end marker */
    wave_starts[0] = 0;
    
    for (uint8_t i = 1; i <= wave_count; i++) {
        uint32_t base_pos = (base_wave_period * i);
        if (i < wave_count) {
            /* Add random offset ±15% of base_wave_period */
            int32_t max_offset = (int32_t)(base_wave_period * 15) / 100;
            int32_t offset = ((rand_timing >> (i * 4)) % (max_offset * 2 + 1)) - max_offset;
            wave_starts[i] = (uint32_t)((int32_t)base_pos + offset);
        } else {
            wave_starts[i] = state->period_ms;  /* End of cycle */
        }
    }
    
    /* Find which wave we're in */
    uint8_t current_wave = 0;
    for (uint8_t i = 0; i < wave_count; i++) {
        if (cycle_pos >= wave_starts[i] && cycle_pos < wave_starts[i + 1]) {
            current_wave = i;
            break;
        }
    }
    
    /* Calculate position within current wave (0 to wave_duration) */
    uint32_t wave_start = wave_starts[current_wave];
    uint32_t wave_end = wave_starts[current_wave + 1];
    uint32_t wave_duration = wave_end - wave_start;
    uint32_t wave_pos = cycle_pos - wave_start;
    uint32_t wave_half = wave_duration / 2;
    
    /* Random peak height for this wave (80-100%) */
    uint8_t wave_peak = 80 + ((rand_heights >> (current_wave * 3)) % 21);
    
    /* Triangle wave: 20% -> peak -> 20% */
    uint8_t primary_factor;
    if (wave_pos < wave_half) {
        /* Rising */
        primary_factor = 20 + ((wave_pos * (wave_peak - 20)) / wave_half);
    } else {
        /* Falling */
        primary_factor = wave_peak - (((wave_pos - wave_half) * (wave_peak - 20)) / wave_half);
    }
    
    /* Secondary wave: faster ripple overlay (±12%) for surface texture */
    uint32_t ripple_period = state->period_ms / 7;  /* 7x faster ripple */
    uint32_t ripple_pos = elapsed % ripple_period;
    uint32_t ripple_half = ripple_period / 2;
    int8_t ripple_offset;
    
    if (ripple_pos < ripple_half) {
        ripple_offset = (int8_t)((ripple_pos * 12) / ripple_half);
    } else {
        ripple_offset = (int8_t)(12 - (((ripple_pos - ripple_half) * 24) / ripple_half));
    }
    
    /* Combine waves with clamping */
    int16_t brightness_factor = (int16_t)primary_factor + ripple_offset;
    if (brightness_factor < 15) brightness_factor = 15;
    if (brightness_factor > 100) brightness_factor = 100;
    
    /* Color variation between cyan and deep blue */
    uint32_t color_cycle = elapsed % (state->period_ms * 2);
    uint8_t color_progress = (color_cycle * 255) / (state->period_ms * 2);
    
    uint8_t r = 0;
    uint8_t g, b;
    
    /* Oscillate between deep blue (g=80) and cyan (g=180) */
    if (color_progress < 128) {
        g = 80 + ((color_progress * 100) / 128);
    } else {
        g = 180 - (((color_progress - 128) * 100) / 127);
    }
    b = 255;
    
    uint8_t effective_brightness = (state->brightness * (uint8_t)brightness_factor) / 100;
    render_color(r, g, b, effective_brightness);
}
