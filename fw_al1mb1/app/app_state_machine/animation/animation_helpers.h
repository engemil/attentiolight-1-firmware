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
 * @file    animation_helpers.h
 * @brief   Shared helper functions for animation effects.
 *
 * @details Provides common utilities used by multiple effect implementations:
 *          - HSV to RGB color conversion
 *          - Pseudo-random number generation
 *          - Brightness application
 *          - Color rendering
 */

#ifndef ANIMATION_HELPERS_H
#define ANIMATION_HELPERS_H

#include <stdint.h>
#include <stdbool.h>
#include "animation_commands.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================*/
/* Random Number Generation                                                  */
/*===========================================================================*/

/**
 * @brief   Seed for the pseudo-random number generator.
 * @details Can be set directly by effects for reproducible sequences.
 */
extern uint32_t effect_random_seed;

/**
 * @brief   Simple pseudo-random number generator for effects.
 * @details Uses a linear congruential generator for reproducible randomness.
 *
 * @return  15-bit pseudo-random value (0-32767).
 */
uint32_t effect_random(void);

/*===========================================================================*/
/* Color Conversion                                                          */
/*===========================================================================*/

/**
 * @brief   Converts HSV to RGB.
 *
 * @param[in] h     Hue (0-359).
 * @param[in] s     Saturation (0-255).
 * @param[in] v     Value/brightness (0-255).
 * @param[out] r    Red output.
 * @param[out] g    Green output.
 * @param[out] b    Blue output.
 */
void hsv_to_rgb(uint16_t h, uint8_t s, uint8_t v,
                uint8_t* r, uint8_t* g, uint8_t* b);

/**
 * @brief   Applies brightness to a color component.
 *
 * @param[in] color      Color component value (0-255).
 * @param[in] brightness Brightness level (0-255).
 *
 * @return  Brightness-adjusted color component.
 */
uint8_t apply_brightness(uint8_t color, uint8_t brightness);

/*===========================================================================*/
/* Rendering                                                                 */
/*===========================================================================*/

/**
 * @brief   Renders current color to LED (with skip-if-unchanged optimization).
 *
 * @param[in] r         Red component (0-255).
 * @param[in] g         Green component (0-255).
 * @param[in] b         Blue component (0-255).
 * @param[in] brightness Brightness level (0-255).
 *
 * @return  true if actually rendered, false if skipped.
 */
bool render_color(uint8_t r, uint8_t g, uint8_t b, uint8_t brightness);

/**
 * @brief   Renders LED off and updates tracking.
 */
void render_off(void);

/**
 * @brief   Force next render (for mode transitions).
 */
void render_force_next(void);

/**
 * @brief   Resets render tracking state.
 * @details Clears last rendered color and forces next render.
 *          Call this when reinitializing the animation system.
 */
void render_reset(void);

/*===========================================================================*/
/* Transition State Tracking                                                 */
/*===========================================================================*/

/**
 * @brief   Last transition state for state-based rendering.
 * @details Used by effects that only render on state changes.
 *          Set to 0xFF to force first render on mode change.
 */
extern uint8_t last_transition_state;

#ifdef __cplusplus
}
#endif

#endif /* ANIMATION_HELPERS_H */
