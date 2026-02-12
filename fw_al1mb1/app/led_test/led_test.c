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
 * Implementation of a simple LED color cycling thread.
 */

#include "led_test.h"
#include "ws2812b_led_driver.h"

/*===========================================================================*/
/* Driver Local Definitions                                                  */
/*===========================================================================*/

/**
 * @brief   RGB color structure for the color cycle.
 */
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} led_color_t;

/**
 * @brief   Color cycle array (Red, Green, Blue).
 * @note    Using 0x80 as brightness level (LSB first for WS2812B).
 */
static const led_color_t color_cycle[] = {
    {0x80, 0x00, 0x00},  /* Red   */
    {0x00, 0x80, 0x00},  /* Green */
    {0x00, 0x00, 0x80}   /* Blue  */
};

#define COLOR_CYCLE_COUNT   (sizeof(color_cycle) / sizeof(color_cycle[0]))

/*===========================================================================*/
/* Driver Local Variables                                                    */
/*===========================================================================*/

/**
 * @brief   LED test thread working area.
 */
static THD_WORKING_AREA(wa_led_test_thread, LED_TEST_THREAD_WA_SIZE);

/**
 * @brief   LED test thread pointer.
 */
static thread_t *led_test_thread_ptr = NULL;

/**
 * @brief   Binary semaphore for start/stop control.
 */
static binary_semaphore_t led_test_sem;

/**
 * @brief   Driver state.
 */
static volatile led_test_state_t driver_state = LED_TEST_STATE_UNINIT;

/*===========================================================================*/
/* Driver Local Functions                                                    */
/*===========================================================================*/

/**
 * @brief   LED test thread function.
 * @details Cycles through red, green, and blue colors while running.
 */
static THD_FUNCTION(led_test_thread, arg) {
    (void)arg;
    uint8_t color_index = 0;

    chRegSetThreadName("led_test_thd");

    while (true) {
        /* Wait for start signal if stopped */
        if (driver_state != LED_TEST_STATE_RUNNING) {
            chBSemWait(&led_test_sem);
            color_index = 0;  /* Reset to first color on start */
            continue;
        }

        /* Set the current color */
        const led_color_t *color = &color_cycle[color_index];
        ws2812b_led_driver_set_color_rgb_and_render(color->r, color->g, color->b);

        /* Wait for the cycle interval */
        chThdSleepMilliseconds(LED_TEST_CYCLE_INTERVAL_MS);

        /* Advance to next color */
        color_index = (color_index + 1) % COLOR_CYCLE_COUNT;
    }
}

/*===========================================================================*/
/* Driver Public Functions                                                   */
/*===========================================================================*/

uint8_t led_test_init(void) {
    /* Check if already initialized */
    if (driver_state != LED_TEST_STATE_UNINIT) {
        return 1;
    }

    /* Initialize binary semaphore (start taken - not signaled) */
    chBSemObjectInit(&led_test_sem, true);

    /* Create LED test thread */
    led_test_thread_ptr = chThdCreateStatic(wa_led_test_thread,
                                            sizeof(wa_led_test_thread),
                                            LED_TEST_THREAD_PRIORITY,
                                            led_test_thread,
                                            NULL);

    driver_state = LED_TEST_STATE_STOPPED;

    return 0;
}

uint8_t led_test_start(void) {
    /* Check if initialized */
    if (driver_state == LED_TEST_STATE_UNINIT) {
        return 1;
    }

    /* Check if already running */
    if (driver_state == LED_TEST_STATE_RUNNING) {
        return 2;
    }

    driver_state = LED_TEST_STATE_RUNNING;

    /* Signal thread to start */
    chBSemSignal(&led_test_sem);

    return 0;
}

uint8_t led_test_stop(void) {
    /* Check if running */
    if (driver_state != LED_TEST_STATE_RUNNING) {
        return 1;
    }

    driver_state = LED_TEST_STATE_STOPPED;

    return 0;
}

led_test_state_t led_test_get_state(void) {
    return driver_state;
}
