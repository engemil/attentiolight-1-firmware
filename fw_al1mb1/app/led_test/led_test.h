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
 * LED Test Driver for ChibiOS
 *
 * A simple LED test module that cycles through Red, Green, and Blue colors
 * using a dedicated ChibiOS thread.
 */

#ifndef LED_TEST_H
#define LED_TEST_H

#include "ch.h"
#include "hal.h"

/*===========================================================================*/
/* Configuration                                                             */
/*===========================================================================*/

/**
 * @brief   LED test thread working area size in bytes.
 */
#ifndef LED_TEST_THREAD_WA_SIZE
#define LED_TEST_THREAD_WA_SIZE      256
#endif

/**
 * @brief   LED test thread priority.
 * @details Low priority since LED cycling is not time-critical.
 */
#ifndef LED_TEST_THREAD_PRIORITY
#define LED_TEST_THREAD_PRIORITY     (LOWPRIO + 1)
#endif

/**
 * @brief   Interval between color changes in milliseconds.
 */
#ifndef LED_TEST_CYCLE_INTERVAL_MS
#define LED_TEST_CYCLE_INTERVAL_MS   500
#endif

/*===========================================================================*/
/* Driver Data Types                                                         */
/*===========================================================================*/

/**
 * @brief   LED test driver state.
 */
typedef enum {
    LED_TEST_STATE_UNINIT = 0,       /**< Driver not initialized         */
    LED_TEST_STATE_STOPPED,          /**< Initialized but not running    */
    LED_TEST_STATE_RUNNING           /**< Driver active and cycling      */
} led_test_state_t;

/*===========================================================================*/
/* Driver Public API                                                         */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Initializes the LED test driver.
 * @details Creates the LED cycling thread in stopped state.
 *
 * @return  0 on success, 1 if already initialized.
 *
 * @api
 */
uint8_t led_test_init(void);

/**
 * @brief   Starts the LED test driver.
 * @details Begins cycling through colors.
 *
 * @return  0 on success, 1 if not initialized, 2 if already running.
 *
 * @api
 */
uint8_t led_test_start(void);

/**
 * @brief   Stops the LED test driver.
 * @details Stops color cycling. Thread remains alive but idle.
 *
 * @return  0 on success, 1 if not running.
 *
 * @api
 */
uint8_t led_test_stop(void);

/**
 * @brief   Returns the current driver state.
 *
 * @return  Current driver state (UNINIT, STOPPED, or RUNNING).
 *
 * @api
 */
led_test_state_t led_test_get_state(void);

#ifdef __cplusplus
}
#endif

#endif /* LED_TEST_H */
