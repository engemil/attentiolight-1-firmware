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
 * @file    effect_day_night.c
 * @brief   Day/night cycle effect implementation.
 */

#include "effect_day_night.h"
#include "animation_helpers.h"

/*===========================================================================*/
/* Effect Implementation                                                     */
/*===========================================================================*/

void process_day_night(const anim_state_t *state) {
    /* Use pre-computed elapsed_ms from animation thread.
     * This handles 16-bit systime_t overflow correctly for long periods
     * (up to 24 hours) by using incremental delta tracking. */
    uint32_t cycle_pos = state->elapsed_ms % state->period_ms;

    /* Calculate progress through cycle as 0-65535 (16-bit) for maximum precision */
    uint32_t progress_16bit = (cycle_pos * 65536UL) / state->period_ms;
    
    /* 12 Waypoints for smooth day/night cycle:
     * 
     * WP0  0%:   Night (sustained)  - Dark blue      (10,15,60)    20%
     * WP1  8%:   Night end          - Dark blue      (10,15,60)    20%
     * WP2  14%:  Pre-dawn           - Navy/purple    (30,25,80)    28%
     * WP3  22%:  Dawn glow          - Orange/pink    (200,80,60)   55%
     * WP4  30%:  Sunrise            - Warm orange    (240,140,70)  80%
     * WP5  38%:  Morning            - Bright warm    (255,210,160) 95%
     * WP6  50%:  Midday (sustained) - Bright white   (255,245,220) 100%
     * WP7  62%:  Midday end         - Bright white   (255,245,220) 100%
     * WP8  70%:  Afternoon          - Warm           (255,200,150) 92%
     * WP9  78%:  Sunset             - Orange         (235,110,50)  75%
     * WP10 86%:  Dusk glow          - Orange/purple  (180,70,90)   45%
     * WP11 92%:  Twilight           - Navy/purple    (40,30,90)    28%
     * WP12 100%: Night (loop)       - Dark blue      (10,15,60)    20%
     */
    
    /* Waypoint positions (as 16-bit values: percentage * 655.36) */
    #define WP0   0       /*  0% */
    #define WP1   5243    /*  8% */
    #define WP2   9175    /* 14% */
    #define WP3   14418   /* 22% */
    #define WP4   19661   /* 30% */
    #define WP5   24904   /* 38% */
    #define WP6   32768   /* 50% */
    #define WP7   40632   /* 62% */
    #define WP8   45875   /* 70% */
    #define WP9   51118   /* 78% */
    #define WP10  56361   /* 86% */
    #define WP11  60293   /* 92% */
    #define WP12  65535   /* 100% */
    
    /* Waypoint color/brightness values */
    typedef struct { uint8_t r, g, b, bright; } wp_color_t;
    static const wp_color_t waypoints[] = {
        { 10,  15,  60,  20},  /* WP0:  Night */
        { 10,  15,  60,  20},  /* WP1:  Night end */
        { 30,  25,  80,  28},  /* WP2:  Pre-dawn */
        {200,  80,  60,  55},  /* WP3:  Dawn glow */
        {240, 140,  70,  80},  /* WP4:  Sunrise */
        {255, 210, 160,  95},  /* WP5:  Morning */
        {255, 245, 220, 100},  /* WP6:  Midday */
        {255, 245, 220, 100},  /* WP7:  Midday end */
        {255, 200, 150,  92},  /* WP8:  Afternoon */
        {235, 110,  50,  75},  /* WP9:  Sunset */
        {180,  70,  90,  45},  /* WP10: Dusk glow */
        { 40,  30,  90,  28},  /* WP11: Twilight */
        { 10,  15,  60,  20},  /* WP12: Night (loop) */
    };
    
    /* Waypoint thresholds array for cleaner code */
    static const uint16_t wp_pos[] = {
        WP0, WP1, WP2, WP3, WP4, WP5, WP6, WP7, WP8, WP9, WP10, WP11, WP12
    };
    
    /* Find which segment we're in (default to last segment for edge case
     * when progress_16bit == 65535, which would otherwise leave seg at 0) */
    uint8_t seg = 11;
    for (uint8_t i = 1; i < 12; i++) {
        if (progress_16bit < wp_pos[i]) {
            seg = i - 1;
            break;
        }
    }
    
    /* Calculate interpolation progress within this segment (0-255) */
    uint32_t seg_start = wp_pos[seg];
    uint32_t seg_end = wp_pos[seg + 1];
    uint32_t seg_progress = ((progress_16bit - seg_start) * 256) / (seg_end - seg_start);
    
    /* Interpolate between waypoints */
    const wp_color_t *wp_from = &waypoints[seg];
    const wp_color_t *wp_to = &waypoints[seg + 1];
    
    uint8_t r = wp_from->r + ((int16_t)(wp_to->r - wp_from->r) * (int16_t)seg_progress) / 256;
    uint8_t g = wp_from->g + ((int16_t)(wp_to->g - wp_from->g) * (int16_t)seg_progress) / 256;
    uint8_t b = wp_from->b + ((int16_t)(wp_to->b - wp_from->b) * (int16_t)seg_progress) / 256;
    uint8_t brightness_factor = wp_from->bright + 
                                ((int16_t)(wp_to->bright - wp_from->bright) * (int16_t)seg_progress) / 256;
    
    #undef WP0
    #undef WP1
    #undef WP2
    #undef WP3
    #undef WP4
    #undef WP5
    #undef WP6
    #undef WP7
    #undef WP8
    #undef WP9
    #undef WP10
    #undef WP11
    #undef WP12
    
    uint8_t effective_brightness = (state->brightness * brightness_factor) / 100;
    render_color(r, g, b, effective_brightness);
}
