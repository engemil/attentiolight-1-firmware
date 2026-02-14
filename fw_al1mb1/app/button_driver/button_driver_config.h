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

/*
 * Button Driver Configuration for ChibiOS
 *
 * Configuration macros for the button driver. Override these defines
 * before including the header or copy this file to your project and modify.
 *
 * All defines use #ifndef guards so they can be overridden externally
 * (e.g., in your build system or a project-level config header).
 */

#ifndef BUTTON_DRIVER_CONFIG_H
#define BUTTON_DRIVER_CONFIG_H

/*===========================================================================*/
/* Timing Configuration                                                      */
/*===========================================================================*/

/**
 * @brief   Debounce time in milliseconds.
 * @details Time to wait after edge detection before reading stable state.
 */
#ifndef BTN_DEBOUNCE_MS
#define BTN_DEBOUNCE_MS              20
#endif

/**
 * @brief   Short press maximum duration in milliseconds.
 * @details Releases before this time are considered short presses.
 */
#ifndef BTN_SHORT_MAX_MS
#define BTN_SHORT_MAX_MS             1000
#endif

/**
 * @brief   Long press minimum duration in milliseconds.
 * @details Press duration must reach this to be considered long.
 *          Gap between SHORT_MAX and LONG_MIN is "cancelled" press.
 */
#ifndef BTN_LONG_MIN_MS
#define BTN_LONG_MIN_MS              2000
#endif

/**
 * @brief   Extended press minimum duration in milliseconds.
 * @details Press duration must reach this to be considered extended.
 */
#ifndef BTN_EXTENDED_MIN_MS
#define BTN_EXTENDED_MIN_MS          5000
#endif

/**
 * @brief   Thread polling interval while button is pressed (milliseconds).
 * @details Used to check for threshold crossings while button is held.
 */
#ifndef BTN_POLL_INTERVAL_MS
#define BTN_POLL_INTERVAL_MS         50
#endif

/*===========================================================================*/
/* Thread Configuration                                                      */
/*===========================================================================*/

/**
 * @brief   Button thread working area size in bytes.
 */
#ifndef BTN_THREAD_WA_SIZE
#define BTN_THREAD_WA_SIZE           256
#endif

/**
 * @brief   Button thread priority.
 * @details Low priority since button handling is not time-critical.
 */
#ifndef BTN_THREAD_PRIORITY
#define BTN_THREAD_PRIORITY          (LOWPRIO + 1)
#endif

#endif /* BUTTON_DRIVER_CONFIG_H */
