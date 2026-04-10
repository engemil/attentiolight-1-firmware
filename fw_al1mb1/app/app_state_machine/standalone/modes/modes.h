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
 * @file    modes.h
 * @brief   Operational modes interface and registration.
 */

#ifndef MODES_H
#define MODES_H

#include "app_state_machine.h"
#include "standalone_state.h"

/*===========================================================================*/
/* Mode Interface                                                            */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================*/
/* Standalone Color Palette                                                  */
/*===========================================================================*/

/**
 * @brief   Standalone 12-color palette.
 * @details Used by all standalone modes that need color selection.
 */
extern const uint8_t standalone_color_palette[APP_SM_COLOR_COUNT][3];

/*===========================================================================*/
/* Mode Functions                                                            */
/*===========================================================================*/

/**
 * @brief   Initializes all modes.
 */
void modes_init(void);

/**
 * @brief   Enters the current mode.
 */
void modes_enter_current(void);

/**
 * @brief   Exits the current mode.
 */
void modes_exit_current(void);

/**
 * @brief   Handles short press for current mode.
 */
void modes_on_short_press(void);

/**
 * @brief   Handles long press start for current mode.
 */
void modes_on_long_start(void);

/**
 * @brief   Gets mode operations for a specific mode.
 */
const app_sm_mode_ops_t* modes_get_ops(app_sm_mode_t mode);

/*===========================================================================*/
/* Individual Mode Declarations                                              */
/*===========================================================================*/

/* Solid Color Mode */
extern const app_sm_mode_ops_t mode_solid_color_ops;

/* Brightness Mode */
extern const app_sm_mode_ops_t mode_brightness_ops;

/* Blinking Mode */
extern const app_sm_mode_ops_t mode_blinking_ops;

/* Pulsation Mode */
extern const app_sm_mode_ops_t mode_pulsation_ops;

/* Effects Mode */
extern const app_sm_mode_ops_t mode_effects_ops;

/* Traffic Light Mode */
extern const app_sm_mode_ops_t mode_traffic_light_ops;

/* Night Light Mode */
extern const app_sm_mode_ops_t mode_night_light_ops;

/* External Control Mode */
extern const app_sm_mode_ops_t mode_external_control_ops;

#ifdef __cplusplus
}
#endif

#endif /* MODES_H */
