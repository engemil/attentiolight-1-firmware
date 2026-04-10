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
 * @file    standalone_state.c
 * @brief   Standalone mode shared state definitions.
 *
 * @details Defines the global variables shared across standalone modes.
 *          These were previously in app_state_machine.c but are standalone-
 *          specific and do not belong in the shared SM core.
 */

#include "standalone_state.h"

/*===========================================================================*/
/* Standalone Shared State                                                   */
/*===========================================================================*/

/**
 * @brief   Current operational mode.
 */
app_sm_mode_t current_mode = APP_SM_DEFAULT_MODE;

/**
 * @brief   External control active flag.
 */
bool external_control_active = false;

/**
 * @brief   Saved mode before external control.
 */
app_sm_mode_t saved_mode_before_external = APP_SM_DEFAULT_MODE;

/**
 * @brief   Standalone brightness level.
 */
uint8_t standalone_brightness = APP_SM_DEFAULT_BRIGHTNESS;

/**
 * @brief   Standalone color index (0-11, indexes into color palette).
 */
uint8_t standalone_color_index = APP_SM_DEFAULT_COLOR_INDEX;
