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
 * @file    effect_memory.c
 * @brief   Memory effect implementation.
 */

#include "effect_memory.h"
#include "../../animation/animation_helpers.h"
#include "ch.h"

/*===========================================================================*/
/* Effect Implementation                                                     */
/*===========================================================================*/

void process_memory(const anim_state_t *state) {
    uint32_t now = chVTGetSystemTime();
    uint32_t elapsed = TIME_I2MS(now - state->start_time);
    
    /* Use fixed total cycle to avoid discontinuities from changing random values */
    /* Total cycle = 2x base period (provides good glow + dark ratio) */
    uint32_t fixed_cycle = state->period_ms * 2;
    uint32_t cycle_num = elapsed / fixed_cycle;
    uint32_t local_pos = elapsed % fixed_cycle;
    
    /* Seed random based on cycle number for stable values within each cycle */
    effect_random_seed = cycle_num * 4099 + 7;
    uint32_t rand_timing = effect_random();
    uint32_t rand_color = effect_random();
    
    /* Randomized glow duration within fixed cycle: 30-50% of total cycle */
    /* This leaves 50-70% for dark period */
    uint32_t glow_duration = (fixed_cycle * (30 + (rand_timing % 21))) / 100;
    
    uint8_t brightness_factor;
    
    if (local_pos < glow_duration) {
        /* During glow: fast rise, slow fade (asymmetric) */
        /* Rise phase: 20% of glow duration */
        /* Fall phase: 80% of glow duration */
        uint32_t rise_duration = glow_duration / 5;
        uint32_t fall_duration = glow_duration - rise_duration;
        
        if (local_pos < rise_duration) {
            /* Fast rise: 0 -> 100% brightness */
            brightness_factor = (local_pos * 100) / rise_duration;
        } else {
            /* Slow fade: 100% -> 0 with quadratic decay */
            uint32_t fall_pos = local_pos - rise_duration;
            uint32_t progress = (fall_pos * 255) / fall_duration;
            /* Quadratic decay for more natural slow fade */
            uint32_t inv_progress = 255 - progress;
            brightness_factor = (inv_progress * inv_progress * 100) / (255 * 255);
        }
    } else {
        /* Dark period: completely off */
        brightness_factor = 0;
    }
    
    /* Warm color palette: ambers, soft oranges, warm yellows */
    uint8_t r, g, b;
    uint8_t color_choice = rand_color % 5;
    
    switch (color_choice) {
        case 0:  /* Amber */
            r = 255; g = 140; b = 20;
            break;
        case 1:  /* Soft orange */
            r = 255; g = 100; b = 50;
            break;
        case 2:  /* Warm yellow */
            r = 255; g = 180; b = 60;
            break;
        case 3:  /* Peachy */
            r = 255; g = 160; b = 100;
            break;
        default: /* Soft red */
            r = 255; g = 80; b = 60;
            break;
    }
    
    uint8_t effective_brightness = (state->brightness * brightness_factor) / 100;
    render_color(r, g, b, effective_brightness);
}
