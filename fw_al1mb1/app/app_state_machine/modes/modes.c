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
 * @file    modes.c
 * @brief   Mode management and registration.
 */

#include "modes.h"
#include "../app_state_machine_config.h"

/*===========================================================================*/
/* External References                                                       */
/*===========================================================================*/

extern app_sm_mode_t current_mode;

/*===========================================================================*/
/* Mode Registry                                                             */
/*===========================================================================*/

/**
 * @brief   Array of mode operations, indexed by app_sm_mode_t.
 */
static const app_sm_mode_ops_t* mode_registry[APP_SM_MODE_COUNT] = {
    &mode_solid_color_ops,
    &mode_strength_ops,
    &mode_blinking_ops,
    &mode_pulsation_ops,
    &mode_effects_ops,
    &mode_traffic_light_ops,
    &mode_night_light_ops
};

/*===========================================================================*/
/* Public Functions                                                          */
/*===========================================================================*/

void modes_init(void) {
    /* Modes don't require initialization currently */
}

void modes_enter_current(void) {
    if (current_mode < APP_SM_MODE_COUNT) {
        const app_sm_mode_ops_t* ops = mode_registry[current_mode];
        if (ops && ops->enter) {
            ops->enter();
        }
    }
}

void modes_exit_current(void) {
    if (current_mode < APP_SM_MODE_COUNT) {
        const app_sm_mode_ops_t* ops = mode_registry[current_mode];
        if (ops && ops->exit) {
            ops->exit();
        }
    }
}

void modes_on_short_press(void) {
    if (current_mode < APP_SM_MODE_COUNT) {
        const app_sm_mode_ops_t* ops = mode_registry[current_mode];
        if (ops && ops->on_short_press) {
            ops->on_short_press();
        }
    }
}

void modes_on_long_start(void) {
    if (current_mode < APP_SM_MODE_COUNT) {
        const app_sm_mode_ops_t* ops = mode_registry[current_mode];
        if (ops && ops->on_long_start) {
            ops->on_long_start();
        }
    }
}

const app_sm_mode_ops_t* modes_get_ops(app_sm_mode_t mode) {
    if (mode < APP_SM_MODE_COUNT) {
        return mode_registry[mode];
    }
    return NULL;
}
