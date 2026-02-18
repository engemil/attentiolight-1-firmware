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
 * @file    mode_traffic_light.c
 * @brief   Traffic light mode implementation.
 *
 * @details Cycles through Red -> Yellow -> Green sequence like a traffic
 *          light. Short press toggles between automatic and manual advance.
 */

#include "modes.h"
#include "../animation/animation_thread.h"
#include "../app_state_machine_config.h"
#include "app_debug.h"

/*===========================================================================*/
/* Traffic Light States                                                      */
/*===========================================================================*/

#if (APP_DEBUG_LEVEL >= DBG_LEVEL_INFO)
/**
 * @brief   Traffic state names for debug output.
 */
static const char* const traffic_state_names[3] = {
    "RED", "YELLOW", "GREEN"
};
#endif

typedef enum {
    TRAFFIC_RED = 0,
    TRAFFIC_YELLOW,
    TRAFFIC_GREEN
} traffic_state_t;

/*===========================================================================*/
/* Local Variables                                                           */
/*===========================================================================*/

static bool auto_mode = true;  /* true = automatic cycling, false = manual */
static traffic_state_t manual_state = TRAFFIC_RED;

/* External reference to global brightness */
extern uint8_t global_brightness;

/*===========================================================================*/
/* Local Functions                                                           */
/*===========================================================================*/

/**
 * @brief   Sets manual traffic light color.
 */
static void set_traffic_color(traffic_state_t state) {
    switch (state) {
        case TRAFFIC_RED:
            anim_thread_set_solid(255, 0, 0, global_brightness);
            break;
        case TRAFFIC_YELLOW:
            anim_thread_set_solid(255, 200, 0, global_brightness);
            break;
        case TRAFFIC_GREEN:
            anim_thread_set_solid(0, 255, 0, global_brightness);
            break;
    }
}

/*===========================================================================*/
/* Mode Functions                                                            */
/*===========================================================================*/

static void traffic_light_enter(void) {
    DBG_INFO("MODE TrafficLight: enter mode=%s", auto_mode ? "AUTO" : "MANUAL");
    if (auto_mode) {
        /* Start automatic traffic light animation */
        anim_thread_traffic_light(global_brightness);
    } else {
        /* Show current manual state */
        set_traffic_color(manual_state);
    }
}

static void traffic_light_exit(void) {
    DBG_INFO("MODE TrafficLight: exit");
    
}

static void traffic_light_on_short_press(void) {
    if (auto_mode) {
        /* Switch to manual mode */
        auto_mode = false;
        manual_state = TRAFFIC_RED;
        DBG_INFO("MODE TrafficLight: Set from AUTO to MANUAL (state=%s)",
                 traffic_state_names[manual_state]);
        set_traffic_color(manual_state);
    } else {
        /* Advance to next color in manual mode */
        traffic_state_t old_state = manual_state;
        DBG_UNUSED(old_state);
        manual_state = (traffic_state_t)((manual_state + 1) % 3);
        if (manual_state == TRAFFIC_RED) {
            /* Completed one cycle, switch back to auto */
            auto_mode = true;
            DBG_INFO("MODE TrafficLight: Set from MANUAL %s to AUTO",
                     traffic_state_names[old_state]);
            anim_thread_traffic_light(global_brightness);
        } else {
            DBG_INFO("MODE TrafficLight: Set state from %s to %s",
                     traffic_state_names[old_state], traffic_state_names[manual_state]);
            set_traffic_color(manual_state);
        }
    }
}

static void traffic_light_on_long_start(void) {
    DBG_DEBUG("MODE TrafficLight: long_start");
    /* No special action for long press start in this mode */
}

/*===========================================================================*/
/* Mode Operations Structure                                                 */
/*===========================================================================*/

const app_sm_mode_ops_t mode_traffic_light_ops = {
    .name = "Traffic Light",
    .enter = traffic_light_enter,
    .exit = traffic_light_exit,
    .on_short_press = traffic_light_on_short_press,
    .on_long_start = traffic_light_on_long_start
};
