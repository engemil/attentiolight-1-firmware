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
 * @brief   Processes breathing animation tick (asymmetric: slow in, slower out).
 */
static void process_breathing(void) {
    uint32_t now = chVTGetSystemTime();
    uint32_t elapsed = TIME_I2MS(now - anim_state.start_time);
    uint32_t cycle_pos = elapsed % anim_state.period_ms;

    /* Asymmetric breathing: 40% inhale, 60% exhale */
    uint32_t inhale_time = (anim_state.period_ms * 40) / 100;
    uint8_t brightness_factor;

    if (cycle_pos < inhale_time) {
        /* Inhale (faster rise) - use quadratic ease-in */
        uint32_t progress = (cycle_pos * 255) / inhale_time;
        brightness_factor = (uint8_t)((progress * progress) / 255);
    } else {
        /* Exhale (slower fall) - use exponential ease-out */
        uint32_t exhale_elapsed = cycle_pos - inhale_time;
        uint32_t exhale_duration = anim_state.period_ms - inhale_time;
        uint32_t progress = (exhale_elapsed * 255) / exhale_duration;
        /* Inverse exponential for smooth decay */
        brightness_factor = (uint8_t)(255 - ((progress * progress) / 255));
    }

    uint8_t effective_brightness = (uint8_t)(((uint16_t)anim_state.brightness * brightness_factor) / 255);
    render_color(anim_state.target_r, anim_state.target_g,
                 anim_state.target_b, effective_brightness);
}

/**
 * @brief   Processes candle flicker animation tick.
 */
static void process_candle(void) {
    uint32_t now = chVTGetSystemTime();
    uint32_t elapsed = TIME_I2MS(now - anim_state.start_time);

    /* Flicker every period_ms with randomness */
    uint32_t tick = elapsed / anim_state.period_ms;
    
    /* Use tick as part of random seed for varied but smooth flicker */
    effect_random_seed = tick * 7919 + 1;
    uint32_t rand_val = effect_random();
    
    /* Brightness varies from 60% to 100% */
    uint8_t min_bright = 60;
    uint8_t range = 40;
    uint8_t brightness_pct = min_bright + (rand_val % range);
    
    /* Occasional dim flicker (10% chance) */
    if ((rand_val % 10) == 0) {
        brightness_pct = 40 + (rand_val % 30);
    }
    
    uint8_t effective_brightness = (anim_state.brightness * brightness_pct) / 100;
    
    /* Warm orange/yellow candle color */
    render_color(255, 120 + (rand_val % 40), 20, effective_brightness);
}

/**
 * @brief   Processes fire animation tick.
 */
static void process_fire(void) {
    uint32_t now = chVTGetSystemTime();
    uint32_t elapsed = TIME_I2MS(now - anim_state.start_time);

    uint32_t tick = elapsed / anim_state.period_ms;
    
    effect_random_seed = tick * 6361 + 3;
    uint32_t rand_val = effect_random();
    uint32_t rand_val2 = effect_random();
    
    /* More aggressive flicker than candle (40% to 100%) */
    uint8_t brightness_pct = 40 + (rand_val % 60);
    uint8_t effective_brightness = (anim_state.brightness * brightness_pct) / 100;
    
    /* Color varies between red, orange, and yellow */
    uint8_t r = 255;
    uint8_t g = 60 + (rand_val2 % 140);  /* 60-200 for red to yellow range */
    uint8_t b = (rand_val2 % 30);         /* Hint of blue for depth */
    
    render_color(r, g, b, effective_brightness);
}

/**
 * @brief   Processes lava lamp animation tick (organic slow color morphing).
 */
static void process_lava_lamp(void) {
    uint32_t now = chVTGetSystemTime();
    uint32_t elapsed = TIME_I2MS(now - anim_state.start_time);
    uint32_t cycle_pos = elapsed % anim_state.period_ms;

    /* Slow hue rotation with sine-wave modulation for organic feel */
    uint16_t base_hue = (uint16_t)((cycle_pos * 360) / anim_state.period_ms);
    
    /* Add secondary slower wave for variation */
    uint32_t slow_cycle = elapsed % (anim_state.period_ms * 3);
    uint16_t wave_offset = (uint16_t)((slow_cycle * 60) / (anim_state.period_ms * 3));
    
    uint16_t hue = (base_hue + wave_offset) % 360;
    
    /* Vary saturation slightly for more organic look (200-255) */
    uint8_t saturation = 200 + ((cycle_pos * 55) / anim_state.period_ms) % 56;
    
    uint8_t r, g, b;
    hsv_to_rgb(hue, saturation, 255, &r, &g, &b);
    
    render_color(r, g, b, anim_state.brightness);
}

/**
 * @brief   Processes day/night cycle animation tick.
 */
static void process_day_night(void) {
    uint32_t now = chVTGetSystemTime();
    uint32_t elapsed = TIME_I2MS(now - anim_state.start_time);
    uint32_t cycle_pos = elapsed % anim_state.period_ms;

    /* Divide cycle into phases:
     * 0-20%:   Night (dark blue) -> Sunrise (red/orange)
     * 20-30%:  Sunrise (orange) -> Day (bright warm white)
     * 30-70%:  Day (bright warm white)
     * 70-80%:  Day -> Sunset (orange/red)
     * 80-100%: Sunset (red) -> Night (dark blue)
     */
    uint32_t phase_pct = (cycle_pos * 100) / anim_state.period_ms;
    
    uint8_t r, g, b;
    uint8_t brightness_factor;
    
    if (phase_pct < 20) {
        /* Night to sunrise: dark blue -> deep red/orange */
        uint32_t progress = (phase_pct * 255) / 20;
        r = (progress * 200) / 255;         /* 0 -> 200 */
        g = (progress * 50) / 255;          /* 0 -> 50 */
        b = 40 - (progress * 40) / 255;     /* 40 -> 0 */
        brightness_factor = 30 + (progress * 70) / 255;  /* 30% -> 100% */
    } else if (phase_pct < 30) {
        /* Sunrise to day: red/orange -> warm white */
        uint32_t progress = ((phase_pct - 20) * 255) / 10;
        r = 200 + (progress * 55) / 255;    /* 200 -> 255 */
        g = 50 + (progress * 180) / 255;    /* 50 -> 230 */
        b = (progress * 200) / 255;         /* 0 -> 200 */
        brightness_factor = 100;
    } else if (phase_pct < 70) {
        /* Day: warm bright white */
        r = 255;
        g = 230;
        b = 200;
        brightness_factor = 100;
    } else if (phase_pct < 80) {
        /* Day to sunset: warm white -> orange/red */
        uint32_t progress = ((phase_pct - 70) * 255) / 10;
        r = 255;
        g = 230 - (progress * 180) / 255;   /* 230 -> 50 */
        b = 200 - (progress * 200) / 255;   /* 200 -> 0 */
        brightness_factor = 100;
    } else {
        /* Sunset to night: red -> dark blue */
        uint32_t progress = ((phase_pct - 80) * 255) / 20;
        r = 200 - (progress * 200) / 255;   /* 200 -> 0 */
        g = 50 - (progress * 50) / 255;     /* 50 -> 0 */
        b = (progress * 40) / 255;          /* 0 -> 40 */
        brightness_factor = 100 - (progress * 70) / 255;  /* 100% -> 30% */
    }
    
    uint8_t effective_brightness = (anim_state.brightness * brightness_factor) / 100;
    render_color(r, g, b, effective_brightness);
}

/**
 * @brief   Processes ocean wave animation tick.
 */
static void process_ocean(void) {
    uint32_t now = chVTGetSystemTime();
    uint32_t elapsed = TIME_I2MS(now - anim_state.start_time);
    uint32_t cycle_pos = elapsed % anim_state.period_ms;

    /* Gentle sine wave for brightness (60% to 100%) */
    uint32_t half_period = anim_state.period_ms / 2;
    uint8_t brightness_factor;
    
    if (cycle_pos < half_period) {
        brightness_factor = 60 + ((cycle_pos * 40) / half_period);
    } else {
        brightness_factor = 100 - (((cycle_pos - half_period) * 40) / half_period);
    }
    
    /* Subtle color variation between cyan and deep blue */
    uint8_t color_blend = (cycle_pos * 255) / anim_state.period_ms;
    uint8_t r = 0;
    uint8_t g = 100 + ((128 - color_blend / 2) * 55) / 128;  /* 100-155 */
    uint8_t b = 255;
    
    uint8_t effective_brightness = (anim_state.brightness * brightness_factor) / 100;
    render_color(r, g, b, effective_brightness);
}

/**
 * @brief   Processes northern lights (aurora) animation tick.
 */
static void process_northern_lights(void) {
    uint32_t now = chVTGetSystemTime();
    uint32_t elapsed = TIME_I2MS(now - anim_state.start_time);
    uint32_t cycle_pos = elapsed % anim_state.period_ms;

    /* Multi-hue blending between green, cyan, and purple */
    /* Three main colors: green (120), cyan (180), purple (280) */
    uint32_t third = anim_state.period_ms / 3;
    uint16_t hue;
    
    if (cycle_pos < third) {
        /* Green to cyan */
        hue = 120 + ((cycle_pos * 60) / third);
    } else if (cycle_pos < third * 2) {
        /* Cyan to purple */
        hue = 180 + (((cycle_pos - third) * 100) / third);
    } else {
        /* Purple back to green (wrap through blue) */
        uint32_t progress = ((cycle_pos - third * 2) * 200) / third;
        if (progress < 80) {
            hue = 280 + progress;  /* 280 -> 360 */
        } else {
            hue = (progress - 80) * 120 / 120;  /* 0 -> 120 */
        }
    }
    
    /* Gentle brightness wave */
    uint32_t bright_cycle = elapsed % (anim_state.period_ms / 2);
    uint8_t brightness_factor = 70 + ((bright_cycle * 30) / (anim_state.period_ms / 2));
    
    uint8_t r, g, b;
    hsv_to_rgb(hue % 360, 200, 255, &r, &g, &b);
    
    uint8_t effective_brightness = (anim_state.brightness * brightness_factor) / 100;
    render_color(r, g, b, effective_brightness);
}

/**
 * @brief   Processes thunder storm animation tick.
 */
static void process_thunder_storm(void) {
    uint32_t now = chVTGetSystemTime();
    uint32_t elapsed = TIME_I2MS(now - anim_state.start_time);

    /* Use a tick-based system for pseudo-random lightning timing */
    uint32_t tick = elapsed / 100;  /* 100ms ticks */
    
    effect_random_seed = tick * 8191 + 17;
    uint32_t rand_val = effect_random();
    
    /* Base dark blue-gray storm color */
    uint8_t base_r = 10;
    uint8_t base_g = 15;
    uint8_t base_b = 40;
    
    /* Lightning flash probability based on period (longer period = rarer flashes) */
    uint32_t flash_threshold = 100 - (10000 / anim_state.period_ms);
    if (flash_threshold > 95) flash_threshold = 95;
    if (flash_threshold < 80) flash_threshold = 80;
    
    uint8_t current_state;
    if ((rand_val % 100) > flash_threshold) {
        /* Lightning flash! */
        current_state = 1;
    } else {
        current_state = 0;
    }
    
    /* Only render on state transition */
    if (current_state != last_transition_state) {
        last_transition_state = current_state;
        if (current_state) {
            /* Bright white lightning */
            render_color(255, 255, 255, anim_state.brightness);
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
 */
static void process_memory(void) {
    uint32_t now = chVTGetSystemTime();
    uint32_t elapsed = TIME_I2MS(now - anim_state.start_time);
    
    /* Use variable timing based on pseudo-random intervals */
    uint32_t tick = elapsed / (anim_state.period_ms / 4);
    
    effect_random_seed = tick * 4099 + 7;
    uint32_t rand_timing = effect_random();
    uint32_t rand_color = effect_random();
    uint32_t rand_bright = effect_random();
    
    /* Create irregular glow pattern */
    uint32_t glow_duration = anim_state.period_ms / 2 + (rand_timing % (anim_state.period_ms / 2));
    uint32_t local_pos = elapsed % glow_duration;
    
    /* Soft fade in and out */
    uint8_t brightness_factor;
    uint32_t half_glow = glow_duration / 2;
    
    if (local_pos < half_glow) {
        /* Fade in */
        brightness_factor = (local_pos * 80) / half_glow;
    } else {
        /* Fade out */
        brightness_factor = 80 - (((local_pos - half_glow) * 80) / half_glow);
    }
    
    /* Add base dim glow */
    brightness_factor = 15 + (brightness_factor * 85) / 100;
    
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
    
    /* Slight random brightness variation */
    brightness_factor = brightness_factor + (rand_bright % 20) - 10;
    if (brightness_factor > 100) brightness_factor = 100;
    if (brightness_factor < 10) brightness_factor = 10;
    
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
