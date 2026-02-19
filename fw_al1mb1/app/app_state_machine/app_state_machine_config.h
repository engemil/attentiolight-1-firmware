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
 * @file    app_state_machine_config.h
 * @brief   Configuration defines for the Application State Machine.
 */

#ifndef APP_STATE_MACHINE_CONFIG_H
#define APP_STATE_MACHINE_CONFIG_H

/*===========================================================================*/
/* Thread Configuration                                                      */
/*===========================================================================*/

/**
 * @brief   State machine thread working area size.
 */
#ifndef APP_SM_THREAD_WA_SIZE
#define APP_SM_THREAD_WA_SIZE           512
#endif

/**
 * @brief   State machine thread priority.
 */
#ifndef APP_SM_THREAD_PRIORITY
#define APP_SM_THREAD_PRIORITY          (NORMALPRIO)
#endif

/**
 * @brief   Animation thread working area size.
 */
#ifndef APP_SM_ANIM_THREAD_WA_SIZE
#define APP_SM_ANIM_THREAD_WA_SIZE      512
#endif

/**
 * @brief   Animation thread priority.
 * @details Higher than SM thread for smooth animations.
 */
#ifndef APP_SM_ANIM_THREAD_PRIORITY
#define APP_SM_ANIM_THREAD_PRIORITY     (NORMALPRIO + 1)
#endif

/**
 * @brief   Input event queue size.
 */
#ifndef APP_SM_INPUT_QUEUE_SIZE
#define APP_SM_INPUT_QUEUE_SIZE         8
#endif

/*===========================================================================*/
/* Timing Configuration                                                      */
/*===========================================================================*/

/**
 * @brief   Powerup animation duration in milliseconds.
 */
#ifndef APP_SM_POWERUP_FADE_MS
#define APP_SM_POWERUP_FADE_MS          1000
#endif

/**
 * @brief   Powerdown animation duration in milliseconds.
 */
#ifndef APP_SM_POWERDOWN_FADE_MS
#define APP_SM_POWERDOWN_FADE_MS        500
#endif

/**
 * @brief   Animation update interval in milliseconds.
 * @details 33ms = ~30 FPS, good balance of smoothness and CPU usage.
 *          Reduced from 20ms (50 FPS) to improve timing stability.
 */
#ifndef APP_SM_ANIM_TICK_MS
#define APP_SM_ANIM_TICK_MS             33
#endif

/**
 * @brief   Mode transition feedback duration in milliseconds.
 */
#ifndef APP_SM_MODE_FEEDBACK_MS
#define APP_SM_MODE_FEEDBACK_MS         100
#endif

/**
 * @brief   Brightness/intensity of the mode change feedback flash.
 * @details Value from 0 (off) to 255 (maximum brightness).
 */
#ifndef APP_SM_CHANGE_MODE_FEEDBACK_BRIGHTNESS
#define APP_SM_CHANGE_MODE_FEEDBACK_BRIGHTNESS  128
#endif


/*===========================================================================*/
/* Feature Toggles                                                           */
/*===========================================================================*/

/**
 * @brief   Enable visual feedback on long press start.
 * @details Set to 1 to show feedback when long press threshold is reached.
 *          Set to 0 to disable feedback until release.
 */
#ifndef APP_SM_LONG_PRESS_FEEDBACK
#define APP_SM_LONG_PRESS_FEEDBACK      1
#endif

/**
 * @brief   Enable mode persistence to flash.
 * @details Set to 1 to save current mode to flash on powerdown.
 *          Requires flash driver implementation.
 */
#ifndef APP_SM_PERSIST_MODE
#define APP_SM_PERSIST_MODE             0
#endif

/**
 * @brief   Enable debug output.
 */
#ifndef APP_SM_DEBUG
#define APP_SM_DEBUG                    0
#endif

/*===========================================================================*/
/* Default Values                                                            */
/*===========================================================================*/

/**
 * @brief   Default operational mode.
 */
#ifndef APP_SM_DEFAULT_MODE
#define APP_SM_DEFAULT_MODE             APP_SM_MODE_SOLID_COLOR
#endif

/**
 * @brief   Default brightness level (0-255).
 */
#ifndef APP_SM_DEFAULT_BRIGHTNESS
#define APP_SM_DEFAULT_BRIGHTNESS       128
#endif

/**
 * @brief   Default color index for solid color mode.
 * 
 * @details Index into shared_color_palette (see mode_solid_color.c).
 *          Set to 0 for first color, or any
 */
#ifndef APP_SM_DEFAULT_COLOR_INDEX
#define APP_SM_DEFAULT_COLOR_INDEX      0
#endif

/*===========================================================================*/
/* Color Palette Configuration                                               */
/*===========================================================================*/

/**
 * @brief   Number of colors in the solid color palette.
 */
#define APP_SM_COLOR_COUNT              12

/**
 * @brief   Number of brightness levels in brightness mode.
 */
#define APP_SM_BRIGHTNESS_LEVELS        8

/*===========================================================================*/
/* Animation Configuration                                                   */
/*===========================================================================*/

/**
 * @brief   Default blink interval in milliseconds.
 */
#ifndef APP_SM_BLINK_INTERVAL_MS
#define APP_SM_BLINK_INTERVAL_MS        500
#endif

/**
 * @brief   Default pulse period in milliseconds.
 */
#ifndef APP_SM_PULSE_PERIOD_MS
#define APP_SM_PULSE_PERIOD_MS          2000
#endif

#endif /* APP_STATE_MACHINE_CONFIG_H */
