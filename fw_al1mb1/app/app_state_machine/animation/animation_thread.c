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
        case ANIM_CMD_STROBE:
        case ANIM_CMD_TRAFFIC_LIGHT:
        case ANIM_CMD_COLOR_CYCLE:
        case ANIM_CMD_FLASH_FEEDBACK:
            return RENDER_TRANSITIONS;
        case ANIM_CMD_PULSE:
        case ANIM_CMD_RAINBOW:
        case ANIM_CMD_FADE_IN:
        case ANIM_CMD_FADE_OUT:
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
 * @brief   Processes fade-in animation tick.
 */
static void process_fade_in(void) {
    uint32_t now = chVTGetSystemTime();
    uint32_t elapsed = TIME_I2MS(now - anim_state.start_time);

    if (elapsed >= anim_state.period_ms) {
        /* Fade complete */
        anim_state.current_r = anim_state.target_r;
        anim_state.current_g = anim_state.target_g;
        anim_state.current_b = anim_state.target_b;
        anim_state.current_type = ANIM_CMD_SOLID;
        render_color(anim_state.current_r, anim_state.current_g,
                     anim_state.current_b, anim_state.brightness);
    } else {
        /* Interpolate */
        uint8_t progress = (uint8_t)((elapsed * 255) / anim_state.period_ms);
        uint8_t r = (anim_state.target_r * progress) / 255;
        uint8_t g = (anim_state.target_g * progress) / 255;
        uint8_t b = (anim_state.target_b * progress) / 255;
        anim_state.current_r = r;
        anim_state.current_g = g;
        anim_state.current_b = b;
        render_color(r, g, b, anim_state.brightness);
    }
}

/**
 * @brief   Processes fade-out animation tick.
 */
static void process_fade_out(void) {
    uint32_t now = chVTGetSystemTime();
    uint32_t elapsed = TIME_I2MS(now - anim_state.start_time);

    if (elapsed >= anim_state.period_ms) {
        /* Fade complete */
        anim_state.current_r = 0;
        anim_state.current_g = 0;
        anim_state.current_b = 0;
        anim_state.current_type = ANIM_CMD_STOP;
        anim_state.active = false;
        render_off();
    } else {
        /* Interpolate */
        uint8_t progress = (uint8_t)(255 - ((elapsed * 255) / anim_state.period_ms));
        uint8_t r = (anim_state.target_r * progress) / 255;
        uint8_t g = (anim_state.target_g * progress) / 255;
        uint8_t b = (anim_state.target_b * progress) / 255;
        anim_state.current_r = r;
        anim_state.current_g = g;
        anim_state.current_b = b;
        render_color(r, g, b, anim_state.brightness);
    }
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
 * @brief   Processes strobe animation tick.
 */
static void process_strobe(void) {
    uint32_t now = chVTGetSystemTime();
    uint32_t elapsed = TIME_I2MS(now - anim_state.start_time);
    uint32_t cycle_pos = elapsed % anim_state.period_ms;

    uint8_t current_state = (cycle_pos < (anim_state.period_ms / 10)) ? 1 : 0;

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
                         anim_state.target_b, 255);
        } else {
            /* Return to previous state - just go to stop for now */
            anim_state.current_type = ANIM_CMD_STOP;
            anim_state.active = false;
            render_off();
        }
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
        case ANIM_CMD_FADE_IN:
            process_fade_in();
            break;
        case ANIM_CMD_FADE_OUT:
            process_fade_out();
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
        case ANIM_CMD_STROBE:
            process_strobe();
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
        DBG_ERROR("ANIM init failed - already initialized");
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

    /* Start LED driver */
    ws2812b_led_driver_start();

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

uint8_t anim_thread_fade_in(uint8_t r, uint8_t g, uint8_t b,
                            uint8_t brightness, uint16_t duration_ms) {
    anim_command_t cmd = {
        .type = ANIM_CMD_FADE_IN,
        .r = r,
        .g = g,
        .b = b,
        .brightness = brightness,
        .period_ms = duration_ms,
        .param = 0
    };
    return anim_thread_send_command(&cmd);
}

uint8_t anim_thread_fade_out(uint16_t duration_ms) {
    anim_command_t cmd = {
        .type = ANIM_CMD_FADE_OUT,
        .r = anim_state.current_r,
        .g = anim_state.current_g,
        .b = anim_state.current_b,
        .brightness = anim_state.brightness,
        .period_ms = duration_ms,
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

uint8_t anim_thread_strobe(uint8_t r, uint8_t g, uint8_t b,
                           uint8_t brightness, uint16_t interval_ms) {
    anim_command_t cmd = {
        .type = ANIM_CMD_STROBE,
        .r = r,
        .g = g,
        .b = b,
        .brightness = brightness,
        .period_ms = interval_ms,
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
        .brightness = 255,
        .period_ms = duration_ms,
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
