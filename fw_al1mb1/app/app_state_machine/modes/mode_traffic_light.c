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
 * @details Cycles through Green -> Yellow -> Red sequence like a traffic
 *          light. Short press advances to the next color. Starts on Green.
 */

#include "modes.h"
#include "../animation/animation_thread.h"
#include "../app_state_machine_config.h"
#include "app_debug.h"

/*===========================================================================*/
/* Traffic Light States                                                      */
/*===========================================================================*/

#if (APP_DEBUG_LEVEL >= DBG_LEVEL_DEBUG)
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

static traffic_state_t current_state = TRAFFIC_GREEN;

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
    DBG_DEBUG("MODE TrafficLight: enter state=%s", traffic_state_names[current_state]);
    set_traffic_color(current_state);
}

static void traffic_light_exit(void) {
    DBG_DEBUG("MODE TrafficLight: exit");
    
}

static void traffic_light_on_short_press(void) {
    /* Cycle through colors: GREEN -> YELLOW -> RED -> GREEN */
    traffic_state_t old_state = current_state;
    DBG_UNUSED(old_state);
    
    switch (current_state) {
        case TRAFFIC_GREEN:
            current_state = TRAFFIC_YELLOW;
            break;
        case TRAFFIC_YELLOW:
            current_state = TRAFFIC_RED;
            break;
        case TRAFFIC_RED:
            current_state = TRAFFIC_GREEN;
            break;
    }
    
    DBG_DEBUG("MODE TrafficLight: Set state from %s to %s",
             traffic_state_names[old_state], traffic_state_names[current_state]);
    set_traffic_color(current_state);
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
