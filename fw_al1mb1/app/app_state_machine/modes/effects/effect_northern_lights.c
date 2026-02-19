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
 * @file    effect_northern_lights.c
 * @brief   Northern lights (aurora) effect implementation.
 */

#include "effect_northern_lights.h"
#include "../../animation/animation_helpers.h"
#include "ch.h"

/*===========================================================================*/
/* Effect Implementation                                                     */
/*===========================================================================*/

void process_northern_lights(const anim_state_t *state) {
    uint32_t now = chVTGetSystemTime();
    uint32_t elapsed = TIME_I2MS(now - state->start_time);
    
    /* Super-cycle is 3x the base period for dark/aurora episodes */
    uint32_t super_period = state->period_ms * 3;
    uint32_t super_cycle_num = elapsed / super_period;
    uint32_t super_pos = elapsed % super_period;
    
    /* Randomize phase boundaries for this super-cycle */
    effect_random_seed = super_cycle_num * 6841 + 23;
    uint32_t rand_timing = effect_random();
    
    /* Dark period: 25-40% of super-cycle (randomized) */
    uint32_t dark1_pct = 25 + (rand_timing % 16);  /* 25-40% */
    uint32_t fade_in_pct = 5;   /* 5% fade in (balanced) */
    uint32_t fade_out_pct = 5;  /* 5% fade out (balanced) */
    uint32_t dark2_pct = 10;    /* 10% end dark */
    uint32_t aurora_pct = 100 - dark1_pct - fade_in_pct - fade_out_pct - dark2_pct;
    
    /* Convert percentages to absolute positions */
    uint32_t dark1_end = (super_period * dark1_pct) / 100;
    uint32_t fade_in_end = dark1_end + (super_period * fade_in_pct) / 100;
    uint32_t aurora_end = fade_in_end + (super_period * aurora_pct) / 100;
    uint32_t fade_out_end = aurora_end + (super_period * fade_out_pct) / 100;
    /* dark2 runs from fade_out_end to super_period */
    
    /* Determine current phase and envelope multiplier (0-100) */
    uint8_t envelope;  /* 0 = fully dark, 100 = fully active */
    
    if (super_pos < dark1_end) {
        /* Dark period 1: dim ambient glow */
        envelope = 0;
    } else if (super_pos < fade_in_end) {
        /* Fade in: 0 -> 100 */
        uint32_t fade_pos = super_pos - dark1_end;
        uint32_t fade_duration = fade_in_end - dark1_end;
        envelope = (uint8_t)((fade_pos * 100) / fade_duration);
    } else if (super_pos < aurora_end) {
        /* Active aurora */
        envelope = 100;
    } else if (super_pos < fade_out_end) {
        /* Fade out: 100 -> 0 */
        uint32_t fade_pos = super_pos - aurora_end;
        uint32_t fade_duration = fade_out_end - aurora_end;
        envelope = 100 - (uint8_t)((fade_pos * 100) / fade_duration);
    } else {
        /* Dark period 2: dim ambient glow */
        envelope = 0;
    }
    
    /* === Aurora color calculation (runs always for smooth transitions) === */
    
    /* Primary slow wave: oscillates in green range (100-150 hue) */
    uint32_t slow_cycle_pos = elapsed % state->period_ms;
    uint32_t half_period = state->period_ms / 2;
    uint16_t slow_hue_offset;
    if (slow_cycle_pos < half_period) {
        slow_hue_offset = (slow_cycle_pos * 50) / half_period;
    } else {
        slow_hue_offset = 50 - (((slow_cycle_pos - half_period) * 50) / half_period);
    }
    
    /* Secondary faster wave: adds blue hints (±30 degrees toward blue) */
    uint32_t fast_period = state->period_ms / 4;
    uint32_t fast_cycle_pos = elapsed % fast_period;
    uint32_t fast_half = fast_period / 2;
    int16_t fast_hue_offset;
    if (fast_cycle_pos < fast_half) {
        fast_hue_offset = (fast_cycle_pos * 30) / fast_half;
    } else {
        fast_hue_offset = 30 - (((fast_cycle_pos - fast_half) * 40) / fast_half);
    }
    
    /* Combine hue: base green (110) + offsets */
    int16_t hue = 110 + (int16_t)slow_hue_offset + fast_hue_offset;
    if (hue < 100) hue = 100;
    if (hue > 200) hue = 200;
    
    /* === Brightness calculation === */
    
    /* Base aurora brightness wave: 50-100% during active phase */
    uint32_t bright_period = state->period_ms * 2 / 3;
    uint32_t bright_pos = elapsed % bright_period;
    uint32_t bright_half = bright_period / 2;
    uint8_t aurora_brightness;
    if (bright_pos < bright_half) {
        aurora_brightness = 50 + ((bright_pos * 50) / bright_half);
    } else {
        aurora_brightness = 100 - (((bright_pos - bright_half) * 50) / bright_half);
    }
    
    /* Fast flutter overlay: ±12% for shimmer effect */
    uint32_t flutter_period = state->period_ms / 7;
    uint32_t flutter_pos = elapsed % flutter_period;
    uint32_t flutter_half = flutter_period / 2;
    int8_t flutter;
    if (flutter_pos < flutter_half) {
        flutter = (int8_t)((flutter_pos * 12) / flutter_half);
    } else {
        flutter = (int8_t)(12 - (((flutter_pos - flutter_half) * 24) / flutter_half));
    }
    
    int16_t active_brightness = (int16_t)aurora_brightness + flutter;
    if (active_brightness < 40) active_brightness = 40;
    if (active_brightness > 100) active_brightness = 100;
    
    /* === Apply envelope to blend between dark and aurora === */
    /* Dark = 8-12% dim glow, Active = full aurora brightness */
    
    uint8_t dim_brightness = 8 + ((elapsed / 500) % 5);  /* Subtle 8-12% variation */
    
    /* Blend: brightness = dim + (active - dim) * envelope / 100 */
    uint8_t final_brightness;
    if (envelope == 0) {
        final_brightness = dim_brightness;
    } else if (envelope == 100) {
        final_brightness = (uint8_t)active_brightness;
    } else {
        final_brightness = dim_brightness + 
            (((uint16_t)((uint8_t)active_brightness - dim_brightness) * envelope) / 100);
    }
    
    uint8_t r, g, b;
    hsv_to_rgb((uint16_t)hue, 220, 255, &r, &g, &b);
    
    uint8_t effective_brightness = (state->brightness * final_brightness) / 100;
    render_color(r, g, b, effective_brightness);
}
