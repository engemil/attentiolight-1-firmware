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
 * @file    animation_thread.h
 * @brief   Animation thread interface for smooth LED animations.
 *
 * @details The animation thread runs at a higher priority than the state
 *          machine thread to ensure smooth, jitter-free animations. Commands
 *          are sent via a mailbox and executed continuously until a new
 *          command is received.
 */

#ifndef ANIMATION_THREAD_H
#define ANIMATION_THREAD_H

#include "ch.h"
#include "hal.h"
#include <stdbool.h>

#include "animation_commands.h"

/*===========================================================================*/
/* Public API                                                                */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Initializes the animation thread.
 *
 * @return  0 on success, 1 if already initialized.
 *
 * @api
 */
uint8_t anim_thread_init(void);

/**
 * @brief   Starts the animation thread.
 *
 * @return  0 on success, 1 if not initialized, 2 if already running.
 *
 * @api
 */
uint8_t anim_thread_start(void);

/**
 * @brief   Stops the animation thread.
 *
 * @return  0 on success, 1 if not running.
 *
 * @api
 */
uint8_t anim_thread_stop(void);

/**
 * @brief   Sends an animation command.
 * @details Non-blocking. Command is queued and executed by animation thread.
 *
 * @param[in] cmd       Pointer to animation command.
 *
 * @return  0 on success, 1 if queue full.
 *
 * @api
 */
uint8_t anim_thread_send_command(const anim_command_t* cmd);

/**
 * @brief   Sets solid color immediately.
 * @details Convenience function for setting a static color.
 *
 * @param[in] r         Red component (0-255).
 * @param[in] g         Green component (0-255).
 * @param[in] b         Blue component (0-255).
 * @param[in] brightness Brightness level (0-255).
 *
 * @return  0 on success.
 *
 * @api
 */
uint8_t anim_thread_set_solid(uint8_t r, uint8_t g, uint8_t b, uint8_t brightness);

/**
 * @brief   Starts a fade-in animation.
 *
 * @param[in] r         Target red component.
 * @param[in] g         Target green component.
 * @param[in] b         Target blue component.
 * @param[in] brightness Target brightness.
 * @param[in] duration_ms Fade duration in milliseconds.
 *
 * @return  0 on success.
 *
 * @api
 */
uint8_t anim_thread_fade_in(uint8_t r, uint8_t g, uint8_t b,
                            uint8_t brightness, uint16_t duration_ms);

/**
 * @brief   Starts a fade-out animation.
 *
 * @param[in] duration_ms Fade duration in milliseconds.
 *
 * @return  0 on success.
 *
 * @api
 */
uint8_t anim_thread_fade_out(uint16_t duration_ms);

/**
 * @brief   Starts a blink animation.
 *
 * @param[in] r         Color red component.
 * @param[in] g         Color green component.
 * @param[in] b         Color blue component.
 * @param[in] brightness Brightness level.
 * @param[in] interval_ms Blink interval in milliseconds.
 *
 * @return  0 on success.
 *
 * @api
 */
uint8_t anim_thread_blink(uint8_t r, uint8_t g, uint8_t b,
                          uint8_t brightness, uint16_t interval_ms);

/**
 * @brief   Starts a pulse/breathing animation.
 *
 * @param[in] r         Color red component.
 * @param[in] g         Color green component.
 * @param[in] b         Color blue component.
 * @param[in] brightness Max brightness level.
 * @param[in] period_ms Pulse period in milliseconds.
 *
 * @return  0 on success.
 *
 * @api
 */
uint8_t anim_thread_pulse(uint8_t r, uint8_t g, uint8_t b,
                          uint8_t brightness, uint16_t period_ms);

/**
 * @brief   Starts a rainbow animation.
 *
 * @param[in] brightness Brightness level.
 * @param[in] period_ms Full rainbow cycle period in milliseconds.
 *
 * @return  0 on success.
 *
 * @api
 */
uint8_t anim_thread_rainbow(uint8_t brightness, uint16_t period_ms);

/**
 * @brief   Starts a strobe animation.
 *
 * @param[in] r         Color red component.
 * @param[in] g         Color green component.
 * @param[in] b         Color blue component.
 * @param[in] brightness Brightness level.
 * @param[in] interval_ms Strobe interval in milliseconds.
 *
 * @return  0 on success.
 *
 * @api
 */
uint8_t anim_thread_strobe(uint8_t r, uint8_t g, uint8_t b,
                           uint8_t brightness, uint16_t interval_ms);

/**
 * @brief   Starts a color cycle animation.
 *
 * @param[in] brightness Brightness level.
 * @param[in] interval_ms Interval between colors in milliseconds.
 *
 * @return  0 on success.
 *
 * @api
 */
uint8_t anim_thread_color_cycle(uint8_t brightness, uint16_t interval_ms);

/**
 * @brief   Starts a traffic light animation.
 *
 * @param[in] brightness Brightness level.
 *
 * @return  0 on success.
 *
 * @api
 */
uint8_t anim_thread_traffic_light(uint8_t brightness);

/**
 * @brief   Triggers a quick flash feedback.
 * @details Used for button press feedback. Non-interrupting.
 *
 * @param[in] r         Flash color red.
 * @param[in] g         Flash color green.
 * @param[in] b         Flash color blue.
 * @param[in] duration_ms Flash duration in milliseconds.
 *
 * @return  0 on success.
 *
 * @api
 */
uint8_t anim_thread_flash_feedback(uint8_t r, uint8_t g, uint8_t b,
                                   uint16_t duration_ms);

/**
 * @brief   Stops current animation and turns off LEDs.
 *
 * @return  0 on success.
 *
 * @api
 */
uint8_t anim_thread_off(void);

/**
 * @brief   Checks if animation thread is running.
 *
 * @return  true if running, false otherwise.
 *
 * @api
 */
bool anim_thread_is_running(void);

/**
 * @brief   Gets current animation type.
 *
 * @return  Current animation command type.
 *
 * @api
 */
anim_cmd_type_t anim_thread_get_current_type(void);

#ifdef __cplusplus
}
#endif

#endif /* ANIMATION_THREAD_H */
