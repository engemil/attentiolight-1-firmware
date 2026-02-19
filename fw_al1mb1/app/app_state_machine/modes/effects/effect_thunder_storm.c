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
 * @file    effect_thunder_storm.c
 * @brief   Thunder storm effect implementation.
 */

#include "effect_thunder_storm.h"
#include "../../animation/animation_helpers.h"
#include "ch.h"

/*===========================================================================*/
/* Effect Implementation                                                     */
/*===========================================================================*/

void process_thunder_storm(const anim_state_t *state) {
    uint32_t now = chVTGetSystemTime();
    uint32_t elapsed = TIME_I2MS(now - state->start_time);

    /* Base dark blue-gray storm color */
    uint8_t base_r = 10;
    uint8_t base_g = 15;
    uint8_t base_b = 40;
    
    /* Use tick-based system for lightning timing */
    uint32_t tick = elapsed / 100;  /* 100ms base ticks */
    
    effect_random_seed = tick * 8191 + 17;
    uint32_t rand_val = effect_random();
    uint32_t rand_flash_count = effect_random();
    uint32_t rand_brightness = effect_random();
    
    /* Lightning strike probability based on period */
    uint32_t flash_threshold = 100 - (10000 / state->period_ms);
    if (flash_threshold > 95) flash_threshold = 95;
    if (flash_threshold < 85) flash_threshold = 85;
    
    /* Check for new lightning strike initiation */
    uint8_t strike_active = 0;
    uint8_t flash_brightness = 0;
    
    if ((rand_val % 100) > flash_threshold) {
        /* Lightning strike! Determine number of flashes (2-3) */
        uint8_t num_flashes = 2 + (rand_flash_count % 2);  /* 2 or 3 flashes */
        
        /* Each flash sequence takes ~150ms total (50ms flash + 50ms gap, repeated) */
        uint32_t strike_elapsed = elapsed % 200;  /* Window for the strike */
        uint32_t flash_period = 200 / num_flashes;
        uint32_t flash_pos = strike_elapsed % flash_period;
        
        /* Flash is on for first 40% of each flash period */
        if (flash_pos < (flash_period * 40 / 100)) {
            strike_active = 1;
            
            /* Variable brightness per flash: first flash brightest, subsequent dimmer */
            uint8_t flash_index = strike_elapsed / flash_period;
            uint8_t base_bright = 100 - (flash_index * 15);  /* 100%, 85%, 70% */
            
            /* Add random variation ±20% */
            int8_t variation = (rand_brightness % 41) - 20;
            int16_t final_bright = (int16_t)base_bright + variation;
            if (final_bright < 50) final_bright = 50;
            if (final_bright > 100) final_bright = 100;
            
            flash_brightness = (uint8_t)final_bright;
        }
    }
    
    /* State-based rendering for efficiency */
    uint8_t current_state = strike_active ? (1 + flash_brightness) : 0;
    
    if (current_state != last_transition_state) {
        last_transition_state = current_state;
        
        if (strike_active) {
            /* Lightning flash with variable brightness */
            uint8_t effective = (state->brightness * flash_brightness) / 100;
            /* Slightly blue-white lightning */
            render_color(255, 255, 255, effective);
        } else {
            /* Dark storm background */
            render_color(base_r, base_g, base_b, state->brightness / 3);
        }
    }
}
