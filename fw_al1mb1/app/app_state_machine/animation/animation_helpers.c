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
 * @file    animation_helpers.c
 * @brief   Shared helper functions for animation effects.
 */

#include "animation_helpers.h"
#include "ws2812b_led_driver.h"

/*===========================================================================*/
/* Module Variables                                                          */
/*===========================================================================*/

/**
 * @brief   Seed for the pseudo-random number generator.
 */
uint32_t effect_random_seed = 12345;

/**
 * @brief   Last transition state for state-based rendering.
 */
uint8_t last_transition_state = 0xFF;

/**
 * @brief   Last rendered color (for skip-if-unchanged optimization).
 */
static uint8_t last_rendered_r = 0;
static uint8_t last_rendered_g = 0;
static uint8_t last_rendered_b = 0;
static bool force_render = true;  /* Force first render */

/*===========================================================================*/
/* Random Number Generation                                                  */
/*===========================================================================*/

uint32_t effect_random(void) {
    effect_random_seed = effect_random_seed * 1103515245 + 12345;
    return (effect_random_seed >> 16) & 0x7FFF;
}

void effect_random_window(uint32_t elapsed_ms,
                          uint32_t tick_ms,
                          uint32_t seed_mul,
                          uint32_t seed_add,
                          uint32_t min_interval_ms,
                          uint32_t range_interval_ms,
                          uint32_t *interval_ms,
                          uint32_t *anchor_ms,
                          uint32_t *since_anchor_ms) {
    uint32_t tick = elapsed_ms / tick_ms;
    effect_random_seed = tick * seed_mul + seed_add;
    uint32_t rnd = effect_random();

    uint32_t chosen_interval = min_interval_ms;
    if (range_interval_ms > 0) {
        chosen_interval += rnd % range_interval_ms;
    }

    uint32_t chosen_anchor = (elapsed_ms / chosen_interval) * chosen_interval;

    if (interval_ms != NULL) {
        *interval_ms = chosen_interval;
    }
    if (anchor_ms != NULL) {
        *anchor_ms = chosen_anchor;
    }
    if (since_anchor_ms != NULL) {
        *since_anchor_ms = elapsed_ms - chosen_anchor;
    }
}

/*===========================================================================*/
/* Color Conversion                                                          */
/*===========================================================================*/

void hsv_to_rgb(uint16_t h, uint8_t s, uint8_t v,
                uint8_t* r, uint8_t* g, uint8_t* b) {
    if (s == 0) {
        *r = *g = *b = v;
        return;
    }

    uint8_t region = h / 60;
    uint8_t remainder = (h - (region * 60)) * 255 / 60;

    uint8_t p = (v * (255 - s)) / 255;
    uint8_t q = (v * (255 - ((s * remainder) / 255))) / 255;
    uint8_t t = (v * (255 - ((s * (255 - remainder)) / 255))) / 255;

    switch (region) {
        case 0:  *r = v; *g = t; *b = p; break;
        case 1:  *r = q; *g = v; *b = p; break;
        case 2:  *r = p; *g = v; *b = t; break;
        case 3:  *r = p; *g = q; *b = v; break;
        case 4:  *r = t; *g = p; *b = v; break;
        default: *r = v; *g = p; *b = q; break;
    }
}

uint8_t apply_brightness(uint8_t color, uint8_t brightness) {
    return (uint8_t)(((uint16_t)color * brightness) / 255);
}

uint8_t triangle_wave_u8(uint32_t pos,
                         uint32_t period,
                         uint8_t min_value,
                         uint8_t max_value) {
    if (period < 2 || max_value <= min_value) {
        return min_value;
    }

    uint32_t half = period / 2;
    uint32_t span = (uint32_t)max_value - min_value;

    if (pos < half) {
        return (uint8_t)(min_value + ((pos * span) / half));
    }

    return (uint8_t)(max_value - (((pos - half) * span) / half));
}

uint8_t two_phase_transition_u8(uint8_t start,
                                uint8_t mid,
                                uint8_t end,
                                uint32_t since_anchor_ms,
                                uint32_t phase1_ms,
                                uint32_t phase2_ms) {
    if (since_anchor_ms < phase1_ms) {
        int32_t progress = (int32_t)((since_anchor_ms * 100U) / phase1_ms);
        int32_t diff = (int32_t)mid - (int32_t)start;
        return (uint8_t)((int32_t)start + ((diff * progress) / 100));
    }

    uint32_t phase2_elapsed = since_anchor_ms - phase1_ms;
    if (phase2_elapsed > phase2_ms) {
        phase2_elapsed = phase2_ms;
    }
    int32_t progress = (int32_t)((phase2_elapsed * 100U) / phase2_ms);
    int32_t diff = (int32_t)end - (int32_t)mid;
    return (uint8_t)((int32_t)mid + ((diff * progress) / 100));
}

/*===========================================================================*/
/* Rendering                                                                 */
/*===========================================================================*/

bool render_color(uint8_t r, uint8_t g, uint8_t b, uint8_t brightness) {
    uint8_t r_out = apply_brightness(r, brightness);
    uint8_t g_out = apply_brightness(g, brightness);
    uint8_t b_out = apply_brightness(b, brightness);
    
    /* Skip render if color unchanged (unless forced) */
    if (!force_render &&
        r_out == last_rendered_r &&
        g_out == last_rendered_g &&
        b_out == last_rendered_b) {
        return false;
    }
    
    ws2812b_led_driver_set_color_rgb_and_render(r_out, g_out, b_out);
    last_rendered_r = r_out;
    last_rendered_g = g_out;
    last_rendered_b = b_out;
    force_render = false;
    return true;
}

void render_off(void) {
    ws2812b_led_driver_set_color_rgb_and_render(0, 0, 0);
    last_rendered_r = 0;
    last_rendered_g = 0;
    last_rendered_b = 0;
}

void render_force_next(void) {
    force_render = true;
}

void render_reset(void) {
    last_rendered_r = 0;
    last_rendered_g = 0;
    last_rendered_b = 0;
    force_render = true;
}
