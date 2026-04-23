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
 * @file    effect_fire.c
 * @brief   Fire effect implementation.
 */

#include "effect_fire.h"
#include "animation_helpers.h"

/*===========================================================================*/
/* Effect Implementation                                                     */
/*===========================================================================*/

void process_fire(const anim_state_t *state) {
    /* Use pre-computed elapsed_ms from animation thread */
    uint32_t elapsed = state->elapsed_ms;

    /* === Slow base undulation (like glowing embers, ~2 second cycle) === */
    uint32_t base_period = 2000;  /* 2 seconds */
    uint32_t base_pos = elapsed % base_period;
    uint8_t base_brightness = triangle_wave_u8(base_pos, base_period, 70, 90);
    
    /* === Random flicker/flare events (every 0.5-2 seconds) === */
    uint32_t flicker_interval;
    uint32_t anchor_time;
    uint32_t time_since_anchor;
    effect_random_window(elapsed,
                         500,
                         6361,
                         17,
                         500,
                         1500,
                         &flicker_interval,
                         &anchor_time,
                         &time_since_anchor);
    
    /* Flicker duration: ~200ms total (quick flare up + settle down) */
    #define FIRE_FLARE_MS       80
    #define FIRE_SETTLE_MS      120
    #define FIRE_FLICKER_TOTAL  (FIRE_FLARE_MS + FIRE_SETTLE_MS)
    
    uint8_t final_brightness = base_brightness;
    
    /* Check if we're in a flicker event */
    if (time_since_anchor < FIRE_FLICKER_TOTAL) {
        effect_random_seed = anchor_time * 8761 + 7;
        uint32_t rand_intensity = effect_random();
        uint8_t flare_boost = 10 + (rand_intensity % 16);  /* 10-25% boost */
        
        uint8_t flare_max = base_brightness + flare_boost;
        if (flare_max > 100) flare_max = 100;
        
        final_brightness = two_phase_transition_u8(base_brightness,
                                                   flare_max,
                                                   base_brightness,
                                                   time_since_anchor,
                                                   FIRE_FLARE_MS,
                                                   FIRE_SETTLE_MS);
    }
    
    #undef FIRE_FLARE_MS
    #undef FIRE_SETTLE_MS
    #undef FIRE_FLICKER_TOTAL
    
    /* === Micro-flickers: small rapid variations for organic movement === */
    /* Add subtle 3-8% brightness jitter every ~50ms */
    uint32_t micro_tick = elapsed / 50;
    effect_random_seed = micro_tick * 2749 + 31;
    uint32_t rand_micro = effect_random();
    int8_t micro_jitter = (rand_micro % 11) - 5;  /* -5 to +5 */
    
    int16_t adjusted_brightness = (int16_t)final_brightness + micro_jitter;
    if (adjusted_brightness < 60) adjusted_brightness = 60;
    if (adjusted_brightness > 100) adjusted_brightness = 100;
    final_brightness = (uint8_t)adjusted_brightness;
    
    /* === Smooth color interpolation between random target colors === */
    /* Color transition period: 400ms per color, smooth interpolation */
    #define FIRE_COLOR_PERIOD_MS  400
    
    uint32_t color_cycle = elapsed / FIRE_COLOR_PERIOD_MS;
    uint32_t color_pos = elapsed % FIRE_COLOR_PERIOD_MS;
    
    /* Get "from" color (current cycle) */
    effect_random_seed = color_cycle * 4253 + 11;
    uint32_t rand_from = effect_random();
    uint8_t g_from;
    uint8_t type_from = rand_from % 10;
    if (type_from < 4) {
        g_from = 20 + (rand_from % 45);       /* 20-65: deep red */
    } else if (type_from < 8) {
        g_from = 70 + (rand_from % 50);       /* 70-120: orange */
    } else {
        g_from = 130 + (rand_from % 50);      /* 130-180: yellow accent */
    }
    
    /* Get "to" color (next cycle) */
    effect_random_seed = (color_cycle + 1) * 4253 + 11;
    uint32_t rand_to = effect_random();
    uint8_t g_to;
    uint8_t type_to = rand_to % 10;
    if (type_to < 4) {
        g_to = 20 + (rand_to % 45);
    } else if (type_to < 8) {
        g_to = 70 + (rand_to % 50);
    } else {
        g_to = 130 + (rand_to % 50);
    }
    
    /* Smooth linear interpolation between colors */
    uint8_t interp = (color_pos * 255) / FIRE_COLOR_PERIOD_MS;
    uint8_t g = g_from + (((int16_t)g_to - (int16_t)g_from) * interp) / 255;
    
    #undef FIRE_COLOR_PERIOD_MS
    
    uint8_t effective_brightness = (state->brightness * final_brightness) / 100;
    render_color(255, g, 0, effective_brightness);
}
