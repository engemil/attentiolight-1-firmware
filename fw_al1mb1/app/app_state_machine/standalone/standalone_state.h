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
 * @file    standalone_state.h
 * @brief   Standalone mode shared state declarations.
 *
 * @details Declares the global variables that are shared across standalone
 *          modes (brightness, color index, current mode, external control
 *          flag). These are defined in standalone_state.c.
 */

#ifndef STANDALONE_STATE_H
#define STANDALONE_STATE_H

#include "app_state_machine.h"
#include "standalone_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================*/
/* Standalone Shared State                                                   */
/*===========================================================================*/

/**
 * @brief   Current operational mode.
 */
extern app_sm_mode_t current_mode;

/**
 * @brief   External control active flag.
 */
extern bool external_control_active;

/**
 * @brief   Saved mode before external control.
 */
extern app_sm_mode_t saved_mode_before_external;

/**
 * @brief   Standalone brightness level (0-255).
 * @details Set by brightness mode, used by solid, blink, and pulse modes.
 */
extern uint8_t standalone_brightness;

/**
 * @brief   Standalone color index (0-11, indexes into standalone_color_palette).
 * @details Set by solid color mode, used by brightness, blink, and pulse modes.
 */
extern uint8_t standalone_color_index;

#ifdef __cplusplus
}
#endif

#endif /* STANDALONE_STATE_H */
