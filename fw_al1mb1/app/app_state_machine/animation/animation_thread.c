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
#include "animation_helpers.h"
#include "ws2812b_led_driver.h"
#include "rt_config.h"
#include "standalone_config.h"
#include "modes.h"
#include "mode_traffic_light_config.h"
#include "state_powerup_config.h"
#include "state_powerdown_config.h"

/* Effect mode includes */
#include "effect_breathing.h"
#include "effect_candle.h"
#include "effect_color_cycle.h"
#include "effect_day_night.h"
#include "effect_fire.h"
#include "effect_health_pulse.h"
#include "effect_lava_lamp.h"
#include "effect_memory.h"
#include "effect_northern_lights.h"
#include "effect_ocean.h"
#include "effect_police.h"
#include "effect_rainbow.h"
#include "effect_thunder_storm.h"

/* Log support */
#include "app_log.h"

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

/*===========================================================================*/
/* Local Functions                                                           */
/*===========================================================================*/

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
    uint32_t cycle_pos = anim_state.elapsed_ms % anim_state.period_ms;

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
    uint32_t cycle_pos = anim_state.elapsed_ms % anim_state.period_ms;

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
 * @brief   Processes traffic light animation tick.
 */
static void process_traffic_light(void) {
    uint32_t total_period = APP_SM_TRAFFIC_RED_MS + APP_SM_TRAFFIC_YELLOW_MS +
                            APP_SM_TRAFFIC_GREEN_MS;
    uint32_t cycle_pos = anim_state.elapsed_ms % total_period;

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
    uint32_t elapsed = anim_state.elapsed_ms;

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
    uint32_t elapsed = anim_state.elapsed_ms;

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
    uint32_t elapsed = anim_state.elapsed_ms;

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
    anim_state.last_update_time = anim_state.start_time;
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

    /* Update elapsed_ms using incremental delta tracking.
     * This handles 16-bit systime_t overflow correctly for long periods
     * by accumulating small deltas rather than computing total elapsed.
     * elapsed_ms will overflow after ~49.7 days of continuous operation,
     * which is acceptable for this application. */
    systime_t now = chVTGetSystemTime();
    sysinterval_t delta_ticks = chTimeDiffX(anim_state.last_update_time, now);
    anim_state.last_update_time = now;
    anim_state.elapsed_ms += TIME_I2MS(delta_ticks);

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
            process_rainbow(&anim_state);
            break;
        case ANIM_CMD_COLOR_CYCLE:
            process_color_cycle(&anim_state, &last_transition_state);
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
            process_breathing(&anim_state);
            break;
        case ANIM_CMD_CANDLE:
            process_candle(&anim_state);
            break;
        case ANIM_CMD_FIRE:
            process_fire(&anim_state);
            break;
        case ANIM_CMD_LAVA_LAMP:
            process_lava_lamp(&anim_state);
            break;
        case ANIM_CMD_DAY_NIGHT:
            process_day_night(&anim_state);
            break;
        case ANIM_CMD_OCEAN:
            process_ocean(&anim_state);
            break;
        case ANIM_CMD_NORTHERN_LIGHTS:
            process_northern_lights(&anim_state);
            break;
        case ANIM_CMD_THUNDER_STORM:
            process_thunder_storm(&anim_state);
            break;
        case ANIM_CMD_POLICE:
            process_police(&anim_state);
            break;
        case ANIM_CMD_HEALTH_PULSE:
            process_health_pulse(&anim_state);
            break;
        case ANIM_CMD_MEMORY:
            process_memory(&anim_state);
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
    chVTSet(&anim_tick_vt, TIME_MS2I(RT_ANIMATION_TICK_MS), anim_tick_callback, NULL);

    while (!chThdShouldTerminateX()) {
        /* Wait for events (tick or command notification) */
        eventmask_t events = chEvtWaitAnyTimeout(ANIM_EVT_TICK | ANIM_EVT_CMD, 
                                                  TIME_MS2I(RT_ANIMATION_TICK_MS * 2));

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
            chVTSet(&anim_tick_vt, TIME_MS2I(RT_ANIMATION_TICK_MS), anim_tick_callback, NULL);
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
        LOG_WARN("ANIM init failed - already initialized");
        return 1;
    }

    /* Initialize mailbox */
    chMBObjectInit(&anim_mailbox, anim_mailbox_buffer, ANIM_CMD_MAILBOX_SIZE);

    /* Initialize animation state */
    anim_state.current_type = ANIM_CMD_STOP;
    anim_state.active = false;
    anim_state.brightness = APP_SM_DEFAULT_BRIGHTNESS;

    anim_thread_state = ANIM_THREAD_STOPPED;
    LOG_DEBUG("ANIM init OK");
    return 0;
}

uint8_t anim_thread_start(void) {
    if (anim_thread_state == ANIM_THREAD_UNINIT) {
        LOG_ERROR("ANIM start failed - not initialized");
        return 1;
    }
    if (anim_thread_state == ANIM_THREAD_RUNNING) {
        LOG_WARN("ANIM start - already running");
        return 2;
    }

    /* Initialize virtual timer object */
    chVTObjectInit(&anim_tick_vt);

    /* Initialize (and Start) LED driver */
    ws2812b_led_driver_init();

    /* Reset render tracking */
    render_reset();

    /* Create animation thread */
    anim_thread_ref = chThdCreateStatic(wa_anim_thread, sizeof(wa_anim_thread),
                                        RT_ANIMATION_THREAD_PRIORITY,
                                        anim_thread_func, NULL);

    anim_thread_state = ANIM_THREAD_RUNNING;
    LOG_DEBUG("ANIM started");
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

void anim_thread_get_target_rgb(uint8_t *r, uint8_t *g, uint8_t *b) {
    if (r) *r = anim_state.target_r;
    if (g) *g = anim_state.target_g;
    if (b) *b = anim_state.target_b;
}

uint8_t anim_thread_get_brightness(void) {
    return anim_state.brightness;
}

bool anim_thread_is_active(void) {
    return anim_state.active;
}
