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
 * @file    effect_candle.c
 * @brief   Candle flicker effect implementation.
 */

#include "effect_candle.h"
#include "animation_helpers.h"
#include "ch.h"

/*===========================================================================*/
/* Effect Implementation                                                     */
/*===========================================================================*/

void process_candle(const anim_state_t *state) {
    systime_t now = chVTGetSystemTime();
    sysinterval_t elapsed_ticks = chTimeDiffX(state->start_time, now);
    uint32_t elapsed = TIME_I2MS(elapsed_ticks);

    /* Warm orange/yellow candle color */
    uint8_t base_r = 255;
    uint8_t base_g = 140;  /* Warm orange */
    uint8_t base_b = 20;
    
    /* === Slow base drift (gentle breathing, ~3 second cycle) === */
    uint32_t drift_period = 3000;  /* 3 seconds */
    uint32_t drift_pos = elapsed % drift_period;
    uint8_t drift_brightness;
    
    /* Gentle sine-like drift between 80% and 95% */
    if (drift_pos < drift_period / 2) {
        drift_brightness = 80 + ((drift_pos * 15) / (drift_period / 2));
    } else {
        drift_brightness = 95 - (((drift_pos - drift_period / 2) * 15) / (drift_period / 2));
    }
    
    /* === Random flicker events (every 1-5 seconds) === */
    /* Use 1-second windows to determine if a flicker should occur */
    uint32_t second_tick = elapsed / 1000;
    effect_random_seed = second_tick * 7919 + 13;
    uint32_t rand_interval = effect_random();
    
    /* Determine next flicker time: random 1-5 seconds from last anchor */
    uint32_t flicker_interval = 1000 + (rand_interval % 4000);  /* 1000-5000ms */
    uint32_t anchor_time = (elapsed / flicker_interval) * flicker_interval;
    uint32_t time_since_anchor = elapsed - anchor_time;
    
    /* Flicker duration: ~150ms total (50ms dip + 100ms recovery) */
    #define FLICKER_DIP_MS      50
    #define FLICKER_RECOVER_MS  100
    #define FLICKER_TOTAL_MS    (FLICKER_DIP_MS + FLICKER_RECOVER_MS)
    
    uint8_t final_brightness = drift_brightness;
    
    /* Check if we're in a flicker event (occurs at start of each interval) */
    if (time_since_anchor < FLICKER_TOTAL_MS) {
        /* Calculate flicker depth: subtle dip (down 15-25% from current) */
        effect_random_seed = anchor_time * 4253 + 7;
        uint32_t rand_depth = effect_random();
        uint8_t flicker_depth = 15 + (rand_depth % 11);  /* 15-25% dip */
        
        uint8_t flicker_min = (drift_brightness > flicker_depth) ? 
                              (drift_brightness - flicker_depth) : 55;
        
        if (time_since_anchor < FLICKER_DIP_MS) {
            /* Quick dip down */
            uint32_t dip_progress = (time_since_anchor * 100) / FLICKER_DIP_MS;
            final_brightness = drift_brightness - 
                               ((drift_brightness - flicker_min) * dip_progress) / 100;
        } else {
            /* Slower recovery back up */
            uint32_t recover_elapsed = time_since_anchor - FLICKER_DIP_MS;
            uint32_t recover_progress = (recover_elapsed * 100) / FLICKER_RECOVER_MS;
            final_brightness = flicker_min + 
                               ((drift_brightness - flicker_min) * recover_progress) / 100;
        }
    }
    
    #undef FLICKER_DIP_MS
    #undef FLICKER_RECOVER_MS
    #undef FLICKER_TOTAL_MS
    
    /* Slight color warmth variation based on brightness */
    uint8_t g_adjust = base_g + ((final_brightness - 80) * 2);  /* Slightly more yellow when brighter */
    if (g_adjust > 160) g_adjust = 160;
    
    uint8_t effective_brightness = (state->brightness * final_brightness) / 100;
    render_color(base_r, g_adjust, base_b, effective_brightness);
}
