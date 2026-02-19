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
 * @file    animation_thread.c
 * @brief   Animation thread implementation.
 */

#include "animation_thread.h"
#include "ws2812b_led_driver.h"
#include "../app_state_machine_config.h"

/* Debug support */
#include "app_debug.h"

/*===========================================================================*/
/* Local Definitions                                                         */
/*===========================================================================*/

/**
 * @brief   Animation thread states.
 */
typedef enum {
    ANIM_THREAD_UNINIT = 0,
    ANIM_THREAD_STOPPED,
    ANIM_THREAD_RUNNING
} anim_thread_state_t;

/**
 * @brief   Command mailbox size.
 */
#define ANIM_CMD_MAILBOX_SIZE   4

/*===========================================================================*/
/* Local Variables                                                           */
/*===========================================================================*/

/**
 * @brief   Thread working area.
 */
static THD_WORKING_AREA(wa_anim_thread, APP_SM_ANIM_THREAD_WA_SIZE);

/**
 * @brief   Thread reference.
 */
static thread_t* anim_thread_ref = NULL;

/**
 * @brief   Thread state.
 */
static volatile anim_thread_state_t anim_thread_state = ANIM_THREAD_UNINIT;

/**
 * @brief   Command mailbox.
 */
static mailbox_t anim_mailbox;

/**
 * @brief   Mailbox buffer.
 */
static msg_t anim_mailbox_buffer[ANIM_CMD_MAILBOX_SIZE];

/**
 * @brief   Current animation state.
 */
static anim_state_t anim_state;

/**
 * @brief   Command storage for mailbox.
 */
static anim_command_t cmd_storage[ANIM_CMD_MAILBOX_SIZE];
static volatile uint8_t cmd_write_idx = 0;

/**
 * @brief   Virtual timer for deterministic tick timing.
 */
static virtual_timer_t anim_tick_vt;

/**
 * @brief   Event flags for thread synchronization.
 */
#define ANIM_EVT_TICK       ((eventmask_t)1)
#define ANIM_EVT_CMD        ((eventmask_t)2)

/**
 * @brief   Last rendered color (for skip-if-unchanged optimization).
 */
static uint8_t last_rendered_r = 0;
static uint8_t last_rendered_g = 0;
static uint8_t last_rendered_b = 0;
static bool force_render = true;  /* Force first render */

/**
 * @brief   Render mode for animation types.
 */
typedef enum {
    RENDER_STATIC,      /* Render once, then idle */
    RENDER_TRANSITIONS, /* Render on internal state changes */
    RENDER_CONTINUOUS   /* Render every tick for smooth animation */
} render_mode_t;

/**
 * @brief   Current render mode (set on command).
 */
static render_mode_t current_render_mode = RENDER_STATIC;

/**
 * @brief   Transition state tracking for RENDER_TRANSITIONS modes.
 * @details Set to 0xFF to force first render on mode change.
 */
static uint8_t last_transition_state = 0xFF;

/**
 * @brief   Color palette for color cycle animation.
 */
static const uint8_t color_palette[APP_SM_COLOR_COUNT][3] = {
    {255,   0,   0},    /* Red          */
    {255, 128,   0},    /* Orange       */
    {255, 255,   0},    /* Yellow       */
    {128, 255,   0},    /* Lime         */
    {  0, 255,   0},    /* Green        */
    {  0, 255, 128},    /* Spring       */
    {  0, 255, 255},    /* Cyan         */
    {  0, 128, 255},    /* Azure        */
    {  0,   0, 255},    /* Blue         */
    {128,   0, 255},    /* Purple       */
    {255,   0, 255},    /* Magenta      */
    {255,   0, 128}     /* Pink         */
};

/*===========================================================================*/
/* Local Functions                                                           */
/*===========================================================================*/

/**
 * @brief   Applies brightness to a color component.
 */
static uint8_t apply_brightness(uint8_t color, uint8_t brightness) {
    return (uint8_t)(((uint16_t)color * brightness) / 255);
}

/**
 * @brief   Converts HSV to RGB.
 * @param[in] h     Hue (0-359).
 * @param[in] s     Saturation (0-255).
 * @param[in] v     Value/brightness (0-255).
 * @param[out] r    Red output.
 * @param[out] g    Green output.
 * @param[out] b    Blue output.
 */
static void hsv_to_rgb(uint16_t h, uint8_t s, uint8_t v,
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

/**
 * @brief   Virtual timer callback for animation tick.
 * @details Called from ISR context to signal the animation thread.
 */
static void anim_tick_callback(virtual_timer_t *vtp, void *arg) {
    (void)vtp;
    (void)arg;
    
    chSysLockFromISR();
    if (anim_thread_ref != NULL) {
        chEvtSignalI(anim_thread_ref, ANIM_EVT_TICK);
    }
    chSysUnlockFromISR();
}

/**
 * @brief   Force next render (for mode transitions).
 */
static void render_force_next(void) {
    force_render = true;
}

/**
 * @brief   Renders LED off and updates tracking.
 */
static void render_off(void) {
    ws2812b_led_driver_set_color_rgb_and_render(0, 0, 0);
    last_rendered_r = 0;
    last_rendered_g = 0;
    last_rendered_b = 0;
}

/**
 * @brief   Gets render mode for animation type.
 */
static render_mode_t get_render_mode(anim_cmd_type_t type) {
    switch (type) {
        case ANIM_CMD_SOLID:
        case ANIM_CMD_STOP:
            return RENDER_STATIC;
        case ANIM_CMD_BLINK:
        case ANIM_CMD_TRAFFIC_LIGHT:
        case ANIM_CMD_COLOR_CYCLE:
        case ANIM_CMD_FLASH_FEEDBACK:
        case ANIM_CMD_THUNDER_STORM:
        case ANIM_CMD_HEALTH_PULSE:
            return RENDER_TRANSITIONS;
        case ANIM_CMD_PULSE:
        case ANIM_CMD_RAINBOW:
        case ANIM_CMD_POWERUP_SEQUENCE:
        case ANIM_CMD_POWERDOWN_SEQUENCE:
        case ANIM_CMD_BREATHING:
        case ANIM_CMD_CANDLE:
        case ANIM_CMD_FIRE:
        case ANIM_CMD_LAVA_LAMP:
        case ANIM_CMD_DAY_NIGHT:
        case ANIM_CMD_OCEAN:
        case ANIM_CMD_NORTHERN_LIGHTS:
        case ANIM_CMD_POLICE:
        case ANIM_CMD_MEMORY:
        default:
            return RENDER_CONTINUOUS;
    }
}

/**
 * @brief   Renders current color to LED (with skip-if-unchanged optimization).
 * @return  true if actually rendered, false if skipped.
 */
static bool render_color(uint8_t r, uint8_t g, uint8_t b, uint8_t brightness) {
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

/**
 * @brief   Processes solid color command.
 */
static void process_solid(void) {
    render_color(anim_state.target_r, anim_state.target_g,
                 anim_state.target_b, anim_state.brightness);
    anim_state.current_r = anim_state.target_r;
    anim_state.current_g = anim_state.target_g;
    anim_state.current_b = anim_state.target_b;
}

/**
 * @brief   Processes blink animation tick.
 */
static void process_blink(void) {
    uint32_t now = chVTGetSystemTime();
    uint32_t elapsed = TIME_I2MS(now - anim_state.start_time);
    uint32_t cycle_pos = elapsed % anim_state.period_ms;

    uint8_t current_state = (cycle_pos < (anim_state.period_ms / 2)) ? 1 : 0;

    /* Only render on state transition */
    if (current_state != last_transition_state) {
        last_transition_state = current_state;
        if (current_state) {
            render_color(anim_state.target_r, anim_state.target_g,
                         anim_state.target_b, anim_state.brightness);
        } else {
            render_off();
        }
    }
}

/**
 * @brief   Processes pulse/breathing animation tick.
 */
static void process_pulse(void) {
    uint32_t now = chVTGetSystemTime();
    uint32_t elapsed = TIME_I2MS(now - anim_state.start_time);
    uint32_t cycle_pos = elapsed % anim_state.period_ms;

    /* Use sine-like curve: 0 -> max -> 0 */
    uint16_t half_period = anim_state.period_ms / 2;
    uint8_t brightness_factor;

    if (cycle_pos < half_period) {
        /* Rising */
        brightness_factor = (uint8_t)((cycle_pos * 255) / half_period);
    } else {
        /* Falling */
        brightness_factor = (uint8_t)(((anim_state.period_ms - cycle_pos) * 255) / half_period);
    }

    uint8_t effective_brightness = (uint8_t)(((uint16_t)anim_state.brightness * brightness_factor) / 255);
    render_color(anim_state.target_r, anim_state.target_g,
                 anim_state.target_b, effective_brightness);
}

/**
 * @brief   Processes rainbow animation tick.
 */
static void process_rainbow(void) {
    uint32_t now = chVTGetSystemTime();
    uint32_t elapsed = TIME_I2MS(now - anim_state.start_time);
    uint32_t cycle_pos = elapsed % anim_state.period_ms;

    /* Map cycle position to hue (0-359) */
    uint16_t hue = (uint16_t)((cycle_pos * 360) / anim_state.period_ms);
    uint8_t r, g, b;
    hsv_to_rgb(hue, 255, 255, &r, &g, &b);

    render_color(r, g, b, anim_state.brightness);
}

/**
 * @brief   Simple pseudo-random number generator for effects.
 * @details Uses a linear congruential generator for reproducible randomness.
 */
static uint32_t effect_random_seed = 12345;

static uint32_t effect_random(void) {
    effect_random_seed = effect_random_seed * 1103515245 + 12345;
    return (effect_random_seed >> 16) & 0x7FFF;
}

/**
 * @brief   Processes breathing animation tick.
 * @details Four-phase breathing: rise -> hold high -> fall -> hold low
 *          Uses configurable timings for each phase.
 */
static void process_breathing(void) {
    uint32_t now = chVTGetSystemTime();
    uint32_t elapsed = TIME_I2MS(now - anim_state.start_time);
    
    /* Calculate total cycle from individual phase durations */
    uint32_t total_cycle = APP_SM_BREATHING_RISE_MS + APP_SM_BREATHING_HOLD_HIGH_MS +
                           APP_SM_BREATHING_FALL_MS + APP_SM_BREATHING_HOLD_LOW_MS;
    uint32_t cycle_pos = elapsed % total_cycle;
    
    /* Minimum brightness for hold-low phase (ambient glow) */
    #define BREATHING_MIN_BRIGHTNESS 5
    
    uint8_t brightness_factor;
    
    if (cycle_pos < APP_SM_BREATHING_RISE_MS) {
        /* Rise phase - quadratic ease-in from MIN to 255 */
        uint32_t progress = (cycle_pos * 255) / APP_SM_BREATHING_RISE_MS;
        uint32_t quadratic = (progress * progress) / 255;  /* 0 -> 255 */
        brightness_factor = BREATHING_MIN_BRIGHTNESS + 
                            (uint8_t)((quadratic * (255 - BREATHING_MIN_BRIGHTNESS)) / 255);
    } else if (cycle_pos < APP_SM_BREATHING_RISE_MS + APP_SM_BREATHING_HOLD_HIGH_MS) {
        /* Hold high phase - stay at maximum */
        brightness_factor = 255;
    } else if (cycle_pos < APP_SM_BREATHING_RISE_MS + APP_SM_BREATHING_HOLD_HIGH_MS + 
                           APP_SM_BREATHING_FALL_MS) {
        /* Fall phase - quadratic ease-out from 255 to MIN (smooth continuous decay) */
        uint32_t fall_elapsed = cycle_pos - APP_SM_BREATHING_RISE_MS - APP_SM_BREATHING_HOLD_HIGH_MS;
        uint32_t progress = (fall_elapsed * 255) / APP_SM_BREATHING_FALL_MS;
        /* Inverse quadratic for smooth decay, scaled to land at MIN_BRIGHTNESS */
        uint32_t inv_progress = 255 - progress;
        uint32_t quadratic = (inv_progress * inv_progress) / 255;  /* 255 -> 0 */
        brightness_factor = BREATHING_MIN_BRIGHTNESS + 
                            (uint8_t)((quadratic * (255 - BREATHING_MIN_BRIGHTNESS)) / 255);
    } else {
        /* Hold low phase - stay at minimum */
        brightness_factor = BREATHING_MIN_BRIGHTNESS;
    }
    
    #undef BREATHING_MIN_BRIGHTNESS

    uint8_t effective_brightness = (uint8_t)(((uint16_t)anim_state.brightness * brightness_factor) / 255);
    render_color(anim_state.target_r, anim_state.target_g,
                 anim_state.target_b, effective_brightness);
}

/**
 * @brief   Processes candle flicker animation tick.
 * @details Natural candle: steady gentle drift with occasional subtle flickers (every 1-5 seconds).
 */
static void process_candle(void) {
    uint32_t now = chVTGetSystemTime();
    uint32_t elapsed = TIME_I2MS(now - anim_state.start_time);

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
    
    uint8_t effective_brightness = (anim_state.brightness * final_brightness) / 100;
    render_color(base_r, g_adjust, base_b, effective_brightness);
}

/**
 * @brief   Processes fire animation tick.
 * @details Natural fire: glowing base with occasional flicker bursts (every 0.5-2 seconds).
 *          Red/orange dominant with occasional yellow highlights, no greenish tint.
 */
static void process_fire(void) {
    uint32_t now = chVTGetSystemTime();
    uint32_t elapsed = TIME_I2MS(now - anim_state.start_time);

    /* === Slow base undulation (like glowing embers, ~2 second cycle) === */
    uint32_t base_period = 2000;  /* 2 seconds */
    uint32_t base_pos = elapsed % base_period;
    uint8_t base_brightness;
    
    /* Gentle triangle wave undulation between 70% and 90% */
    if (base_pos < base_period / 2) {
        base_brightness = 70 + ((base_pos * 20) / (base_period / 2));
    } else {
        base_brightness = 90 - (((base_pos - base_period / 2) * 20) / (base_period / 2));
    }
    
    /* === Random flicker/flare events (every 0.5-2 seconds) === */
    uint32_t half_sec_tick = elapsed / 500;
    effect_random_seed = half_sec_tick * 6361 + 17;
    uint32_t rand_interval = effect_random();
    
    /* Determine flicker interval: random 500-2000ms */
    uint32_t flicker_interval = 500 + (rand_interval % 1500);
    uint32_t anchor_time = (elapsed / flicker_interval) * flicker_interval;
    uint32_t time_since_anchor = elapsed - anchor_time;
    
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
        
        if (time_since_anchor < FIRE_FLARE_MS) {
            /* Quick flare up */
            uint32_t flare_progress = (time_since_anchor * 100) / FIRE_FLARE_MS;
            final_brightness = base_brightness + 
                               ((flare_max - base_brightness) * flare_progress) / 100;
        } else {
            /* Slower settle back down */
            uint32_t settle_elapsed = time_since_anchor - FIRE_FLARE_MS;
            uint32_t settle_progress = (settle_elapsed * 100) / FIRE_SETTLE_MS;
            final_brightness = flare_max - 
                               ((flare_max - base_brightness) * settle_progress) / 100;
        }
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
    
    uint8_t effective_brightness = (anim_state.brightness * final_brightness) / 100;
    render_color(255, g, 0, effective_brightness);
}

/**
 * @brief   Processes lava lamp animation tick (organic slow color morphing).
 * @details Restricted to warm tones: red/orange only (hue 0-45), no yellow/green.
 *          All transitions use triangle waves to avoid discontinuities.
 */
static void process_lava_lamp(void) {
    uint32_t now = chVTGetSystemTime();
    uint32_t elapsed = TIME_I2MS(now - anim_state.start_time);
    uint32_t cycle_pos = elapsed % anim_state.period_ms;
    uint32_t half_period = anim_state.period_ms / 2;

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
    uint32_t slow_period = anim_state.period_ms * 3;
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
    uint32_t bright_period = anim_state.period_ms * 2 / 3;  /* Slightly different rate */
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
    
    uint8_t effective_brightness = (anim_state.brightness * brightness_factor) / 100;
    render_color(r, g, b, effective_brightness);
}

/**
 * @brief   Processes day/night cycle animation tick.
 * @details Uses 12 waypoints for ultra-smooth interpolation with sustained phases.
 *          Includes sustained night/day periods and realistic dawn/dusk transitions.
 */
static void process_day_night(void) {
    uint32_t now = chVTGetSystemTime();
    uint32_t elapsed = TIME_I2MS(now - anim_state.start_time);
    uint32_t cycle_pos = elapsed % anim_state.period_ms;

    /* Calculate progress through cycle as 0-65535 (16-bit) for maximum precision */
    uint32_t progress_16bit = (cycle_pos * 65536UL) / anim_state.period_ms;
    
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
    
    /* Find which segment we're in */
    uint8_t seg = 0;
    for (uint8_t i = 1; i < 13; i++) {
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
    
    uint8_t effective_brightness = (anim_state.brightness * brightness_factor) / 100;
    render_color(r, g, b, effective_brightness);
}

/**
 * @brief   Processes ocean wave animation tick.
 * @details Multiple waves (1-3) per cycle with randomized timing and heights.
 *          Wave count: 50% chance of 2 waves, 25% each for 1 or 3 waves.
 */
static void process_ocean(void) {
    uint32_t now = chVTGetSystemTime();
    uint32_t elapsed = TIME_I2MS(now - anim_state.start_time);
    
    /* Determine which cycle we're in */
    uint32_t cycle_num = elapsed / anim_state.period_ms;
    uint32_t cycle_pos = elapsed % anim_state.period_ms;
    
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
    uint32_t base_wave_period = anim_state.period_ms / wave_count;
    
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
            wave_starts[i] = anim_state.period_ms;  /* End of cycle */
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
    uint32_t ripple_period = anim_state.period_ms / 7;  /* 7x faster ripple */
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
    uint32_t color_cycle = elapsed % (anim_state.period_ms * 2);
    uint8_t color_progress = (color_cycle * 255) / (anim_state.period_ms * 2);
    
    uint8_t r = 0;
    uint8_t g, b;
    
    /* Oscillate between deep blue (g=80) and cyan (g=180) */
    if (color_progress < 128) {
        g = 80 + ((color_progress * 100) / 128);
    } else {
        g = 180 - (((color_progress - 128) * 100) / 127);
    }
    b = 255;
    
    uint8_t effective_brightness = (anim_state.brightness * (uint8_t)brightness_factor) / 100;
    render_color(r, g, b, effective_brightness);
}

/**
 * @brief   Processes northern lights (aurora) animation tick.
 * @details Aurora episodes with dark periods in between. Super-cycle is 3x period.
 *          Dark (dim) -> Fade in -> Active aurora -> Fade out -> Dark
 */
static void process_northern_lights(void) {
    uint32_t now = chVTGetSystemTime();
    uint32_t elapsed = TIME_I2MS(now - anim_state.start_time);
    
    /* Super-cycle is 3x the base period for dark/aurora episodes */
    uint32_t super_period = anim_state.period_ms * 3;
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
    uint32_t slow_cycle_pos = elapsed % anim_state.period_ms;
    uint32_t half_period = anim_state.period_ms / 2;
    uint16_t slow_hue_offset;
    if (slow_cycle_pos < half_period) {
        slow_hue_offset = (slow_cycle_pos * 50) / half_period;
    } else {
        slow_hue_offset = 50 - (((slow_cycle_pos - half_period) * 50) / half_period);
    }
    
    /* Secondary faster wave: adds blue hints (±30 degrees toward blue) */
    uint32_t fast_period = anim_state.period_ms / 4;
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
    uint32_t bright_period = anim_state.period_ms * 2 / 3;
    uint32_t bright_pos = elapsed % bright_period;
    uint32_t bright_half = bright_period / 2;
    uint8_t aurora_brightness;
    if (bright_pos < bright_half) {
        aurora_brightness = 50 + ((bright_pos * 50) / bright_half);
    } else {
        aurora_brightness = 100 - (((bright_pos - bright_half) * 50) / bright_half);
    }
    
    /* Fast flutter overlay: ±12% for shimmer effect */
    uint32_t flutter_period = anim_state.period_ms / 7;
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
    
    uint8_t effective_brightness = (anim_state.brightness * final_brightness) / 100;
    render_color(r, g, b, effective_brightness);
}

/**
 * @brief   Processes thunder storm animation tick.
 * @details Lightning with 2-3 consecutive flashes per strike, variable brightness.
 */
static void process_thunder_storm(void) {
    uint32_t now = chVTGetSystemTime();
    uint32_t elapsed = TIME_I2MS(now - anim_state.start_time);

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
    uint32_t flash_threshold = 100 - (10000 / anim_state.period_ms);
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
            uint8_t effective = (anim_state.brightness * flash_brightness) / 100;
            /* Slightly blue-white lightning */
            render_color(255, 255, 255, effective);
        } else {
            /* Dark storm background */
            render_color(base_r, base_g, base_b, anim_state.brightness / 3);
        }
    }
}

/**
 * @brief   Processes police lights animation tick (smooth transitions).
 */
static void process_police(void) {
    uint32_t now = chVTGetSystemTime();
    uint32_t elapsed = TIME_I2MS(now - anim_state.start_time);
    uint32_t cycle_pos = elapsed % anim_state.period_ms;

    /* Smooth transition: red -> transition -> blue -> transition -> red
     * 0-40%: red with fade
     * 40-50%: red to blue transition
     * 50-90%: blue with fade
     * 90-100%: blue to red transition
     */
    uint32_t phase_pct = (cycle_pos * 100) / anim_state.period_ms;
    
    uint8_t r, g, b;
    
    if (phase_pct < 40) {
        /* Red phase with slight pulse */
        uint8_t pulse = 80 + ((phase_pct * 20) / 40);
        r = 255;
        g = 0;
        b = 0;
        uint8_t effective_brightness = (anim_state.brightness * pulse) / 100;
        render_color(r, g, b, effective_brightness);
    } else if (phase_pct < 50) {
        /* Red to blue transition */
        uint32_t progress = ((phase_pct - 40) * 255) / 10;
        r = 255 - (progress * 255) / 255;
        g = 0;
        b = (progress * 255) / 255;
        render_color(r, g, b, anim_state.brightness);
    } else if (phase_pct < 90) {
        /* Blue phase with slight pulse */
        uint8_t pulse = 80 + (((phase_pct - 50) * 20) / 40);
        r = 0;
        g = 0;
        b = 255;
        uint8_t effective_brightness = (anim_state.brightness * pulse) / 100;
        render_color(r, g, b, effective_brightness);
    } else {
        /* Blue to red transition */
        uint32_t progress = ((phase_pct - 90) * 255) / 10;
        r = (progress * 255) / 255;
        g = 0;
        b = 255 - (progress * 255) / 255;
        render_color(r, g, b, anim_state.brightness);
    }
}

/**
 * @brief   Processes health pulse (heartbeat) animation tick.
 */
static void process_health_pulse(void) {
    uint32_t now = chVTGetSystemTime();
    uint32_t elapsed = TIME_I2MS(now - anim_state.start_time);
    uint32_t cycle_pos = elapsed % anim_state.period_ms;

    /* Heartbeat pattern: quick beat, pause, quick beat, long pause
     * 0-15%:  First beat (up and down)
     * 15-30%: Short pause
     * 30-45%: Second beat (up and down)
     * 45-100%: Long pause
     */
    uint32_t phase_pct = (cycle_pos * 100) / anim_state.period_ms;
    
    uint8_t brightness_factor;
    uint8_t current_state;
    
    if (phase_pct < 8) {
        /* First beat rise */
        brightness_factor = (phase_pct * 100) / 8;
        current_state = 1;
    } else if (phase_pct < 15) {
        /* First beat fall */
        brightness_factor = 100 - (((phase_pct - 8) * 100) / 7);
        current_state = 2;
    } else if (phase_pct < 30) {
        /* Short pause */
        brightness_factor = 10;
        current_state = 3;
    } else if (phase_pct < 38) {
        /* Second beat rise */
        brightness_factor = 10 + (((phase_pct - 30) * 70) / 8);  /* Slightly weaker */
        current_state = 4;
    } else if (phase_pct < 45) {
        /* Second beat fall */
        brightness_factor = 80 - (((phase_pct - 38) * 70) / 7);
        current_state = 5;
    } else {
        /* Long pause */
        brightness_factor = 10;
        current_state = 6;
    }
    
    /* Only render on state transition for efficiency */
    if (current_state != last_transition_state || 
        current_state == 1 || current_state == 2 || 
        current_state == 4 || current_state == 5) {
        last_transition_state = current_state;
        uint8_t effective_brightness = (anim_state.brightness * brightness_factor) / 100;
        /* Red heartbeat color */
        render_color(255, 0, 30, effective_brightness);
    }
}

/**
 * @brief   Processes memory animation tick (random warm soft glows).
 * @details Fast-rise/slow-fade pulses with significant darkness between glows and random timing.
 */
static void process_memory(void) {
    uint32_t now = chVTGetSystemTime();
    uint32_t elapsed = TIME_I2MS(now - anim_state.start_time);
    
    /* Use fixed total cycle to avoid discontinuities from changing random values */
    /* Total cycle = 2x base period (provides good glow + dark ratio) */
    uint32_t fixed_cycle = anim_state.period_ms * 2;
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
    
    uint8_t effective_brightness = (anim_state.brightness * brightness_factor) / 100;
    render_color(r, g, b, effective_brightness);
}

/**
 * @brief   Processes color cycle animation tick.
 */
static void process_color_cycle(void) {
    uint32_t now = chVTGetSystemTime();
    uint32_t elapsed = TIME_I2MS(now - anim_state.start_time);
    uint32_t total_period = anim_state.period_ms * APP_SM_COLOR_COUNT;
    uint32_t cycle_pos = elapsed % total_period;

    /* Determine current color index */
    uint8_t color_idx = (uint8_t)(cycle_pos / anim_state.period_ms);
    if (color_idx >= APP_SM_COLOR_COUNT) {
        color_idx = 0;
    }

    /* Only render on color transition */
    if (color_idx != last_transition_state) {
        last_transition_state = color_idx;
        render_color(color_palette[color_idx][0],
                     color_palette[color_idx][1],
                     color_palette[color_idx][2],
                     anim_state.brightness);
    }
}

/**
 * @brief   Processes traffic light animation tick.
 */
static void process_traffic_light(void) {
    uint32_t now = chVTGetSystemTime();
    uint32_t elapsed = TIME_I2MS(now - anim_state.start_time);

    uint32_t total_period = APP_SM_TRAFFIC_RED_MS + APP_SM_TRAFFIC_YELLOW_MS +
                            APP_SM_TRAFFIC_GREEN_MS;
    uint32_t cycle_pos = elapsed % total_period;

    uint8_t current_state;
    if (cycle_pos < APP_SM_TRAFFIC_RED_MS) {
        current_state = 0;  /* Red */
    } else if (cycle_pos < (APP_SM_TRAFFIC_RED_MS + APP_SM_TRAFFIC_YELLOW_MS)) {
        current_state = 1;  /* Yellow */
    } else {
        current_state = 2;  /* Green */
    }

    /* Only render on state transition */
    if (current_state != last_transition_state) {
        last_transition_state = current_state;
        switch (current_state) {
            case 0: render_color(255, 0, 0, anim_state.brightness); break;
            case 1: render_color(255, 200, 0, anim_state.brightness); break;
            case 2: render_color(0, 255, 0, anim_state.brightness); break;
            default: break;
        }
    }
}

/**
 * @brief   Processes flash feedback (quick flash then return).
 */
static void process_flash_feedback(void) {
    uint32_t now = chVTGetSystemTime();
    uint32_t elapsed = TIME_I2MS(now - anim_state.start_time);

    uint8_t current_state = (elapsed < anim_state.period_ms) ? 1 : 0;

    /* Only render on state transition */
    if (current_state != last_transition_state) {
        last_transition_state = current_state;
        if (current_state) {
            render_color(anim_state.target_r, anim_state.target_g,
                         anim_state.target_b, anim_state.brightness);
        } else {
            /* Return to previous state - just go to stop for now */
            anim_state.current_type = ANIM_CMD_STOP;
            anim_state.active = false;
            render_off();
        }
    }
}

/**
 * @brief   Powerup sequence phase definitions.
 */
typedef enum {
    POWERUP_PHASE_RAINBOW_FADE = 0,     /**< Rainbow with brightness ramp     */
    POWERUP_PHASE_BLUE_FADE,            /**< Fade to solid blue               */
    POWERUP_PHASE_PULSE,                /**< Blue pulse/breathing             */
    POWERUP_PHASE_BLANK,                /**< LED off                          */
    POWERUP_PHASE_COMPLETE              /**< Animation finished               */
} powerup_phase_t;

/**
 * @brief   Powerdown sequence phase definitions.
 */
typedef enum {
    POWERDOWN_PHASE_COLOR_FADE = 0,      /**< Fade to amber                    */
    POWERDOWN_PHASE_PULSE,               /**< Pulse with decreasing intensity  */
    POWERDOWN_PHASE_FINAL_FADE,          /**< Final fade to off                */
    POWERDOWN_PHASE_COMPLETE             /**< Animation finished               */
} powerdown_phase_t;

/**
 * @brief   Processes powerup sequence animation.
 * @details Multi-phase animation:
 *          Phase 0: Rainbow fade-in (brightness ramps 0->255 while cycling colors)
 *          Phase 1: Fade to solid blue
 *          Phase 2: Blue pulse (2 times)
 *          Phase 3: Blank (LED off)
 *          Phase 4: Complete (stop animation)
 */
static void process_powerup_sequence(void) {
    uint32_t now = chVTGetSystemTime();
    uint32_t elapsed = TIME_I2MS(now - anim_state.start_time);

    /* Calculate phase boundaries */
    uint32_t phase1_start = APP_SM_POWERUP_RAINBOW_FADE_MS;
    uint32_t phase2_start = phase1_start + APP_SM_POWERUP_BLUE_FADE_MS;
    uint32_t phase3_start = phase2_start + 
                            (APP_SM_POWERUP_PULSE_PERIOD_MS * APP_SM_POWERUP_PULSE_COUNT);
    uint32_t phase4_start = phase3_start + APP_SM_POWERUP_BLANK_MS;

    if (elapsed < phase1_start) {
        /*==================================================================*/
        /* Phase 0: Rainbow fade-in with accelerating color cycle           */
        /*==================================================================*/
        uint32_t phase_elapsed = elapsed;
        
        /* Calculate brightness ramp (0 -> 255) */
        uint8_t brightness = (uint8_t)((phase_elapsed * 255) / APP_SM_POWERUP_RAINBOW_FADE_MS);
        
        /*
         * Accelerating rainbow: cycle period decreases from START to END.
         * 
         * To calculate hue position with varying speed, we integrate the
         * instantaneous angular velocity over time. For linear period change:
         *   P(t) = P_start + t * (P_end - P_start)
         *   
         * The integral of 360/P(t) from 0 to T gives us total hue rotation.
         * Using the formula: integral = 360 * T * ln(P_start/P_end) / (P_start - P_end)
         * 
         * For embedded efficiency, we use a discrete approximation:
         * Sum small hue increments based on instantaneous period at each point.
         */
        
        /* Normalized progress (0-255 for integer math) */
        uint32_t progress_256 = (phase_elapsed * 256) / APP_SM_POWERUP_RAINBOW_FADE_MS;
        
        /* Interpolate current period: START -> END as progress goes 0 -> 256 */
        uint32_t period_start = APP_SM_POWERUP_RAINBOW_CYCLE_START_MS;
        uint32_t period_end = APP_SM_POWERUP_RAINBOW_CYCLE_END_MS;
        uint32_t current_period = period_start - 
            ((period_start - period_end) * progress_256) / 256;
        
        /*
         * Calculate cumulative hue using trapezoidal approximation.
         * Average period from start to now ≈ (period_start + current_period) / 2
         * Number of cycles = elapsed_time / average_period
         * Hue = (cycles * 360) mod 360
         */
        uint32_t avg_period = (period_start + current_period) / 2;
        
        /* Prevent division by zero */
        if (avg_period < 10) avg_period = 10;
        
        /* Calculate total degrees rotated (can be > 360) */
        uint32_t total_degrees = (phase_elapsed * 360) / avg_period;
        
        /* Wrap to 0-359 */
        uint16_t hue = (uint16_t)(total_degrees % 360);
        
        uint8_t r, g, b;
        hsv_to_rgb(hue, 255, 255, &r, &g, &b);
        
        /* Store current color for next phase transition */
        anim_state.current_r = r;
        anim_state.current_g = g;
        anim_state.current_b = b;
        
        render_color(r, g, b, brightness);
        anim_state.phase = POWERUP_PHASE_RAINBOW_FADE;

    } else if (elapsed < phase2_start) {
        /*==================================================================*/
        /* Phase 1: Fade to solid blue                                      */
        /*==================================================================*/
        uint32_t phase_elapsed = elapsed - phase1_start;
        
        /* Interpolate from current color to blue */
        uint8_t progress = (uint8_t)((phase_elapsed * 255) / APP_SM_POWERUP_BLUE_FADE_MS);
        uint8_t inv_progress = 255 - progress;
        
        /* Linear interpolation: current_color -> blue */
        uint8_t r = (anim_state.current_r * inv_progress) / 255;
        uint8_t g = (anim_state.current_g * inv_progress) / 255;
        uint8_t b = ((anim_state.current_b * inv_progress) + 
                     (APP_SM_POWERUP_BLUE_B * progress)) / 255;
        
        render_color(r, g, b, anim_state.brightness);
        anim_state.phase = POWERUP_PHASE_BLUE_FADE;

    } else if (elapsed < phase3_start) {
        /*==================================================================*/
        /* Phase 2: Blue pulse (breathing effect)                           */
        /*==================================================================*/
        uint32_t phase_elapsed = elapsed - phase2_start;
        uint32_t pulse_period = APP_SM_POWERUP_PULSE_PERIOD_MS;
        uint32_t cycle_pos = phase_elapsed % pulse_period;
        
        /* Triangular wave for breathing: 0 -> max -> 0 */
        uint16_t half_period = pulse_period / 2;
        uint8_t brightness_factor;
        
        if (cycle_pos < half_period) {
            /* Rising */
            brightness_factor = (uint8_t)((cycle_pos * 255) / half_period);
        } else {
            /* Falling */
            brightness_factor = (uint8_t)(((pulse_period - cycle_pos) * 255) / half_period);
        }
        
        uint8_t effective_brightness = (uint8_t)(((uint16_t)anim_state.brightness * brightness_factor) / 255);
        render_color(APP_SM_POWERUP_BLUE_R, APP_SM_POWERUP_BLUE_G, 
                     APP_SM_POWERUP_BLUE_B, effective_brightness);
        anim_state.phase = POWERUP_PHASE_PULSE;

    } else if (elapsed < phase4_start) {
        /*==================================================================*/
        /* Phase 3: Blank (LED off)                                         */
        /*==================================================================*/
        if (anim_state.phase != POWERUP_PHASE_BLANK) {
            render_off();
            anim_state.phase = POWERUP_PHASE_BLANK;
        }

    } else {
        /*==================================================================*/
        /* Phase 4: Complete                                                */
        /*==================================================================*/
        anim_state.current_type = ANIM_CMD_STOP;
        anim_state.active = false;
        anim_state.phase = POWERUP_PHASE_COMPLETE;
        render_off();
    }
}

/**
 * @brief   Processes powerdown sequence animation.
 * @details Multi-phase animation:
 *          Phase 0: Fade to amber color
 *          Phase 1: Slow pulse with decreasing intensity
 *          Phase 2: Final fade to off
 *          Phase 3: Complete (stop animation)
 */
static void process_powerdown_sequence(void) {
    uint32_t now = chVTGetSystemTime();
    uint32_t elapsed = TIME_I2MS(now - anim_state.start_time);

    /* Calculate phase boundaries */
    uint32_t phase1_start = APP_SM_POWERDOWN_COLOR_FADE_MS;
    uint32_t phase2_start = phase1_start + 
                            (APP_SM_POWERDOWN_PULSE_PERIOD_MS * APP_SM_POWERDOWN_PULSE_COUNT);
    uint32_t phase3_start = phase2_start + APP_SM_POWERDOWN_FINAL_FADE_MS;

    if (elapsed < phase1_start) {
        /*==================================================================*/
        /* Phase 0: Fade to amber color                                     */
        /*==================================================================*/
        uint32_t phase_elapsed = elapsed;
        
        /* Interpolate from current color to amber */
        uint8_t progress = (uint8_t)((phase_elapsed * 255) / APP_SM_POWERDOWN_COLOR_FADE_MS);
        uint8_t inv_progress = 255 - progress;
        
        /* Linear interpolation: current_color -> amber */
        uint8_t r = ((anim_state.current_r * inv_progress) + 
                     (APP_SM_POWERDOWN_AMBER_R * progress)) / 255;
        uint8_t g = ((anim_state.current_g * inv_progress) + 
                     (APP_SM_POWERDOWN_AMBER_G * progress)) / 255;
        uint8_t b = ((anim_state.current_b * inv_progress) + 
                     (APP_SM_POWERDOWN_AMBER_B * progress)) / 255;
        
        render_color(r, g, b, anim_state.brightness);
        anim_state.phase = POWERDOWN_PHASE_COLOR_FADE;

    } else if (elapsed < phase2_start) {
        /*==================================================================*/
        /* Phase 1: Slow pulse with decreasing intensity                    */
        /*==================================================================*/
        uint32_t phase_elapsed = elapsed - phase1_start;
        uint32_t pulse_period = APP_SM_POWERDOWN_PULSE_PERIOD_MS;
        
        /* Determine which pulse we're in (0-indexed) */
        uint32_t pulse_index = phase_elapsed / pulse_period;
        uint32_t cycle_pos = phase_elapsed % pulse_period;
        
        /* Calculate the maximum brightness for this pulse cycle.
         * Each pulse is dimmer than the last.
         * Pulse 0: 100% -> 70%, Pulse 1: 70% -> 40%, etc.
         */
        uint8_t max_brightness_pct = 100 - (pulse_index * 30);
        if (max_brightness_pct < 30) {
            max_brightness_pct = 30;  /* Floor at 30% */
        }
        uint8_t max_brightness = (anim_state.brightness * max_brightness_pct) / 100;
        
        /* Triangular wave for breathing: max -> 0 -> next_max
         * We go down first then up to connect smoothly to next pulse
         */
        uint16_t half_period = pulse_period / 2;
        uint8_t brightness_factor;
        
        if (cycle_pos < half_period) {
            /* Falling: max -> 0 */
            brightness_factor = (uint8_t)(((half_period - cycle_pos) * 255) / half_period);
        } else {
            /* Rising: 0 -> next_max (which is dimmer) */
            uint8_t next_max_pct = 100 - ((pulse_index + 1) * 30);
            if (next_max_pct < 30) {
                next_max_pct = 30;
            }
            /* Scale the rise to reach next pulse's max brightness */
            uint8_t rise_target = (next_max_pct * 255) / max_brightness_pct;
            brightness_factor = (uint8_t)(((cycle_pos - half_period) * rise_target) / half_period);
        }
        
        uint8_t effective_brightness = (uint8_t)(((uint16_t)max_brightness * brightness_factor) / 255);
        render_color(APP_SM_POWERDOWN_AMBER_R, APP_SM_POWERDOWN_AMBER_G, 
                     APP_SM_POWERDOWN_AMBER_B, effective_brightness);
        anim_state.phase = POWERDOWN_PHASE_PULSE;

    } else if (elapsed < phase3_start) {
        /*==================================================================*/
        /* Phase 2: Final fade to off                                       */
        /*==================================================================*/
        uint32_t phase_elapsed = elapsed - phase2_start;
        
        /* Start from the final pulse brightness (about 40% of original) */
        uint8_t start_brightness = (anim_state.brightness * 40) / 100;
        
        /* Fade to 0 */
        uint8_t progress = (uint8_t)((phase_elapsed * 255) / APP_SM_POWERDOWN_FINAL_FADE_MS);
        uint8_t effective_brightness = (start_brightness * (255 - progress)) / 255;
        
        render_color(APP_SM_POWERDOWN_AMBER_R, APP_SM_POWERDOWN_AMBER_G,
                     APP_SM_POWERDOWN_AMBER_B, effective_brightness);
        anim_state.phase = POWERDOWN_PHASE_FINAL_FADE;

    } else {
        /*==================================================================*/
        /* Phase 3: Complete                                                */
        /*==================================================================*/
        anim_state.current_type = ANIM_CMD_STOP;
        anim_state.active = false;
        anim_state.phase = POWERDOWN_PHASE_COMPLETE;
        render_off();
    }
}

/**
 * @brief   Processes a new command.
 */
static void process_command(const anim_command_t* cmd) {
    anim_state.current_type = cmd->type;
    anim_state.target_r = cmd->r;
    anim_state.target_g = cmd->g;
    anim_state.target_b = cmd->b;
    anim_state.brightness = cmd->brightness;
    anim_state.period_ms = cmd->period_ms;
    anim_state.start_time = chVTGetSystemTime();
    anim_state.elapsed_ms = 0;
    anim_state.phase = 0;
    anim_state.active = (cmd->type != ANIM_CMD_STOP);

    /* Set render mode for this animation */
    current_render_mode = get_render_mode(cmd->type);

    /* Reset transition state tracking (0xFF forces first render) */
    last_transition_state = 0xFF;

    /* Force render on mode change */
    render_force_next();

    if (cmd->type == ANIM_CMD_STOP) {
        render_off();
    } else if (cmd->type == ANIM_CMD_SOLID) {
        process_solid();
    }
}

/**
 * @brief   Animation tick processing.
 */
static void process_tick(void) {
    if (!anim_state.active) {
        return;
    }

    switch (anim_state.current_type) {
        case ANIM_CMD_SOLID:
            /* Static, no update needed */
            break;
        case ANIM_CMD_BLINK:
            process_blink();
            break;
        case ANIM_CMD_PULSE:
            process_pulse();
            break;
        case ANIM_CMD_RAINBOW:
            process_rainbow();
            break;
        case ANIM_CMD_COLOR_CYCLE:
            process_color_cycle();
            break;
        case ANIM_CMD_TRAFFIC_LIGHT:
            process_traffic_light();
            break;
        case ANIM_CMD_FLASH_FEEDBACK:
            process_flash_feedback();
            break;
        case ANIM_CMD_POWERUP_SEQUENCE:
            process_powerup_sequence();
            break;
        case ANIM_CMD_POWERDOWN_SEQUENCE:
            process_powerdown_sequence();
            break;
        case ANIM_CMD_BREATHING:
            process_breathing();
            break;
        case ANIM_CMD_CANDLE:
            process_candle();
            break;
        case ANIM_CMD_FIRE:
            process_fire();
            break;
        case ANIM_CMD_LAVA_LAMP:
            process_lava_lamp();
            break;
        case ANIM_CMD_DAY_NIGHT:
            process_day_night();
            break;
        case ANIM_CMD_OCEAN:
            process_ocean();
            break;
        case ANIM_CMD_NORTHERN_LIGHTS:
            process_northern_lights();
            break;
        case ANIM_CMD_THUNDER_STORM:
            process_thunder_storm();
            break;
        case ANIM_CMD_POLICE:
            process_police();
            break;
        case ANIM_CMD_HEALTH_PULSE:
            process_health_pulse();
            break;
        case ANIM_CMD_MEMORY:
            process_memory();
            break;
        default:
            break;
    }
}

/**
 * @brief   Animation thread function.
 */
static THD_FUNCTION(anim_thread_func, arg) {
    (void)arg;
    chRegSetThreadName("animation_thread");

    /* Start the virtual timer for first tick */
    chVTSet(&anim_tick_vt, TIME_MS2I(APP_SM_ANIM_TICK_MS), anim_tick_callback, NULL);

    while (!chThdShouldTerminateX()) {
        /* Wait for events (tick or command notification) */
        eventmask_t events = chEvtWaitAnyTimeout(ANIM_EVT_TICK | ANIM_EVT_CMD, 
                                                  TIME_MS2I(APP_SM_ANIM_TICK_MS * 2));

        /* Process any pending commands (non-blocking) */
        msg_t msg;
        while (chMBFetchTimeout(&anim_mailbox, &msg, TIME_IMMEDIATE) == MSG_OK) {
            anim_command_t* cmd = (anim_command_t*)msg;
            process_command(cmd);
        }

        /* Process animation tick if timer fired */
        if (events & ANIM_EVT_TICK) {
            process_tick();
            /* Restart timer for next tick */
            chVTSet(&anim_tick_vt, TIME_MS2I(APP_SM_ANIM_TICK_MS), anim_tick_callback, NULL);
        }
    }

    /* Thread terminating, stop timer and turn off LED */
    chVTReset(&anim_tick_vt);
    ws2812b_led_driver_set_color_rgb_and_render(0, 0, 0);
}

/*===========================================================================*/
/* Public Functions                                                          */
/*===========================================================================*/

uint8_t anim_thread_init(void) {
    if (anim_thread_state != ANIM_THREAD_UNINIT) {
        DBG_WARN("ANIM init failed - already initialized");
        return 1;
    }

    /* Initialize mailbox */
    chMBObjectInit(&anim_mailbox, anim_mailbox_buffer, ANIM_CMD_MAILBOX_SIZE);

    /* Initialize animation state */
    anim_state.current_type = ANIM_CMD_STOP;
    anim_state.active = false;
    anim_state.brightness = APP_SM_DEFAULT_BRIGHTNESS;

    anim_thread_state = ANIM_THREAD_STOPPED;
    DBG_DEBUG("ANIM init OK");
    return 0;
}

uint8_t anim_thread_start(void) {
    if (anim_thread_state == ANIM_THREAD_UNINIT) {
        DBG_ERROR("ANIM start failed - not initialized");
        return 1;
    }
    if (anim_thread_state == ANIM_THREAD_RUNNING) {
        DBG_WARN("ANIM start - already running");
        return 2;
    }

    /* Initialize virtual timer object */
    chVTObjectInit(&anim_tick_vt);

    /* Initialize (and Start) LED driver */
    ws2812b_led_driver_init();

    /* Reset render tracking */
    last_rendered_r = last_rendered_g = last_rendered_b = 0;
    force_render = true;

    /* Create animation thread */
    anim_thread_ref = chThdCreateStatic(wa_anim_thread, sizeof(wa_anim_thread),
                                        APP_SM_ANIM_THREAD_PRIORITY,
                                        anim_thread_func, NULL);

    anim_thread_state = ANIM_THREAD_RUNNING;
    DBG_DEBUG("ANIM started");
    return 0;
}

uint8_t anim_thread_stop(void) {
    if (anim_thread_state != ANIM_THREAD_RUNNING) {
        return 1;
    }

    /* Stop the virtual timer first */
    chVTReset(&anim_tick_vt);

    /* Request thread termination */
    chThdTerminate(anim_thread_ref);
    chThdWait(anim_thread_ref);
    anim_thread_ref = NULL;

    /* Stop LED driver */
    ws2812b_led_driver_stop();

    anim_thread_state = ANIM_THREAD_STOPPED;
    return 0;
}

uint8_t anim_thread_send_command(const anim_command_t* cmd) {
    if (anim_thread_state != ANIM_THREAD_RUNNING) {
        return 1;
    }

    /* Copy command to storage */
    uint8_t idx = cmd_write_idx;
    cmd_write_idx = (cmd_write_idx + 1) % ANIM_CMD_MAILBOX_SIZE;
    cmd_storage[idx] = *cmd;

    /* Post to mailbox */
    msg_t result = chMBPostTimeout(&anim_mailbox, (msg_t)&cmd_storage[idx], TIME_IMMEDIATE);
    return (result == MSG_OK) ? 0 : 1;
}

uint8_t anim_thread_set_solid(uint8_t r, uint8_t g, uint8_t b, uint8_t brightness) {
    anim_command_t cmd = {
        .type = ANIM_CMD_SOLID,
        .r = r,
        .g = g,
        .b = b,
        .brightness = brightness,
        .period_ms = 0,
        .param = 0
    };
    return anim_thread_send_command(&cmd);
}

uint8_t anim_thread_blink(uint8_t r, uint8_t g, uint8_t b,
                          uint8_t brightness, uint16_t interval_ms) {
    anim_command_t cmd = {
        .type = ANIM_CMD_BLINK,
        .r = r,
        .g = g,
        .b = b,
        .brightness = brightness,
        .period_ms = interval_ms,
        .param = 0
    };
    return anim_thread_send_command(&cmd);
}

uint8_t anim_thread_pulse(uint8_t r, uint8_t g, uint8_t b,
                          uint8_t brightness, uint16_t period_ms) {
    anim_command_t cmd = {
        .type = ANIM_CMD_PULSE,
        .r = r,
        .g = g,
        .b = b,
        .brightness = brightness,
        .period_ms = period_ms,
        .param = 0
    };
    return anim_thread_send_command(&cmd);
}

uint8_t anim_thread_rainbow(uint8_t brightness, uint16_t period_ms) {
    anim_command_t cmd = {
        .type = ANIM_CMD_RAINBOW,
        .r = 0,
        .g = 0,
        .b = 0,
        .brightness = brightness,
        .period_ms = period_ms,
        .param = 0
    };
    return anim_thread_send_command(&cmd);
}

uint8_t anim_thread_color_cycle(uint8_t brightness, uint16_t interval_ms) {
    anim_command_t cmd = {
        .type = ANIM_CMD_COLOR_CYCLE,
        .r = 0,
        .g = 0,
        .b = 0,
        .brightness = brightness,
        .period_ms = interval_ms,
        .param = 0
    };
    return anim_thread_send_command(&cmd);
}

uint8_t anim_thread_breathing(uint8_t r, uint8_t g, uint8_t b,
                              uint8_t brightness, uint16_t period_ms) {
    anim_command_t cmd = {
        .type = ANIM_CMD_BREATHING,
        .r = r,
        .g = g,
        .b = b,
        .brightness = brightness,
        .period_ms = period_ms,
        .param = 0
    };
    return anim_thread_send_command(&cmd);
}

uint8_t anim_thread_candle(uint8_t brightness, uint16_t period_ms) {
    anim_command_t cmd = {
        .type = ANIM_CMD_CANDLE,
        .r = 255,
        .g = 140,
        .b = 20,
        .brightness = brightness,
        .period_ms = period_ms,
        .param = 0
    };
    return anim_thread_send_command(&cmd);
}

uint8_t anim_thread_fire(uint8_t brightness, uint16_t period_ms) {
    anim_command_t cmd = {
        .type = ANIM_CMD_FIRE,
        .r = 255,
        .g = 100,
        .b = 0,
        .brightness = brightness,
        .period_ms = period_ms,
        .param = 0
    };
    return anim_thread_send_command(&cmd);
}

uint8_t anim_thread_lava_lamp(uint8_t brightness, uint16_t period_ms) {
    anim_command_t cmd = {
        .type = ANIM_CMD_LAVA_LAMP,
        .r = 0,
        .g = 0,
        .b = 0,
        .brightness = brightness,
        .period_ms = period_ms,
        .param = 0
    };
    return anim_thread_send_command(&cmd);
}

uint8_t anim_thread_day_night(uint8_t brightness, uint16_t period_ms) {
    anim_command_t cmd = {
        .type = ANIM_CMD_DAY_NIGHT,
        .r = 0,
        .g = 0,
        .b = 0,
        .brightness = brightness,
        .period_ms = period_ms,
        .param = 0
    };
    return anim_thread_send_command(&cmd);
}

uint8_t anim_thread_ocean(uint8_t brightness, uint16_t period_ms) {
    anim_command_t cmd = {
        .type = ANIM_CMD_OCEAN,
        .r = 0,
        .g = 128,
        .b = 255,
        .brightness = brightness,
        .period_ms = period_ms,
        .param = 0
    };
    return anim_thread_send_command(&cmd);
}

uint8_t anim_thread_northern_lights(uint8_t brightness, uint16_t period_ms) {
    anim_command_t cmd = {
        .type = ANIM_CMD_NORTHERN_LIGHTS,
        .r = 0,
        .g = 255,
        .b = 128,
        .brightness = brightness,
        .period_ms = period_ms,
        .param = 0
    };
    return anim_thread_send_command(&cmd);
}

uint8_t anim_thread_thunder_storm(uint8_t brightness, uint16_t period_ms) {
    anim_command_t cmd = {
        .type = ANIM_CMD_THUNDER_STORM,
        .r = 10,
        .g = 15,
        .b = 40,
        .brightness = brightness,
        .period_ms = period_ms,
        .param = 0
    };
    return anim_thread_send_command(&cmd);
}

uint8_t anim_thread_police(uint8_t brightness, uint16_t period_ms) {
    anim_command_t cmd = {
        .type = ANIM_CMD_POLICE,
        .r = 255,
        .g = 0,
        .b = 0,
        .brightness = brightness,
        .period_ms = period_ms,
        .param = 0
    };
    return anim_thread_send_command(&cmd);
}

uint8_t anim_thread_health_pulse(uint8_t brightness, uint16_t period_ms) {
    anim_command_t cmd = {
        .type = ANIM_CMD_HEALTH_PULSE,
        .r = 255,
        .g = 0,
        .b = 30,
        .brightness = brightness,
        .period_ms = period_ms,
        .param = 0
    };
    return anim_thread_send_command(&cmd);
}

uint8_t anim_thread_memory(uint8_t brightness, uint16_t period_ms) {
    anim_command_t cmd = {
        .type = ANIM_CMD_MEMORY,
        .r = 255,
        .g = 140,
        .b = 50,
        .brightness = brightness,
        .period_ms = period_ms,
        .param = 0
    };
    return anim_thread_send_command(&cmd);
}

uint8_t anim_thread_traffic_light(uint8_t brightness) {
    anim_command_t cmd = {
        .type = ANIM_CMD_TRAFFIC_LIGHT,
        .r = 0,
        .g = 0,
        .b = 0,
        .brightness = brightness,
        .period_ms = 0,
        .param = 0
    };
    return anim_thread_send_command(&cmd);
}

uint8_t anim_thread_flash_feedback(uint8_t r, uint8_t g, uint8_t b,
                                   uint16_t duration_ms) {
    anim_command_t cmd = {
        .type = ANIM_CMD_FLASH_FEEDBACK,
        .r = r,
        .g = g,
        .b = b,
        .brightness = APP_SM_CHANGE_MODE_FEEDBACK_BRIGHTNESS,
        .period_ms = duration_ms,
        .param = 0
    };
    return anim_thread_send_command(&cmd);
}

uint8_t anim_thread_powerup_sequence(uint8_t brightness) {
    anim_command_t cmd = {
        .type = ANIM_CMD_POWERUP_SEQUENCE,
        .r = APP_SM_POWERUP_BLUE_R,
        .g = APP_SM_POWERUP_BLUE_G,
        .b = APP_SM_POWERUP_BLUE_B,
        .brightness = brightness,
        .period_ms = 0,  /* Not used - timing is internal */
        .param = 0
    };
    return anim_thread_send_command(&cmd);
}

uint8_t anim_thread_powerdown_sequence(uint8_t brightness) {
    anim_command_t cmd = {
        .type = ANIM_CMD_POWERDOWN_SEQUENCE,
        .r = APP_SM_POWERDOWN_AMBER_R,
        .g = APP_SM_POWERDOWN_AMBER_G,
        .b = APP_SM_POWERDOWN_AMBER_B,
        .brightness = brightness,
        .period_ms = 0,  /* Not used - timing is internal */
        .param = 0
    };
    return anim_thread_send_command(&cmd);
}

uint8_t anim_thread_off(void) {
    anim_command_t cmd = {
        .type = ANIM_CMD_STOP,
        .r = 0,
        .g = 0,
        .b = 0,
        .brightness = 0,
        .period_ms = 0,
        .param = 0
    };
    return anim_thread_send_command(&cmd);
}

bool anim_thread_is_running(void) {
    return anim_thread_state == ANIM_THREAD_RUNNING;
}

anim_cmd_type_t anim_thread_get_current_type(void) {
    return anim_state.current_type;
}
