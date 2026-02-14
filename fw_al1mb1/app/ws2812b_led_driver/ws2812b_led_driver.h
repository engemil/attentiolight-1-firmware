/*
MIT License

Copyright (c) 2025 EngEmil

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
 * @file ws2812b_led_driver.h
 * 
 * @brief WS2812B LED Driver.
 * 
 */

#ifndef _WS2812B_LED_DRIVER_
#define _WS2812B_LED_DRIVER_

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "ch.h"
#include "hal.h"

#ifndef STM32C011xx
#ifndef STM32C031xx
#ifndef STM32C051xx
#ifndef STM32C071xx
#ifndef STM32C091xx
#ifndef STM32C092xx
#error "WS2812B LED Driver only supports STM32C0 series"
#endif
#endif
#endif
#endif
#endif
#endif

#if !HAL_USE_PWM
#error "PWM not enabled in halconf.h"
#endif


#if !HAL_USE_PWM
#error "PWM not enabled in halconf.h"
#endif

#if !(STM32_PWM_USE_TIM1 || STM32_PWM_USE_TIM3 || STM32_PWM_USE_TIM14 || STM32_PWM_USE_TIM16 || STM32_PWM_USE_TIM17)
#error "At least one PWM timer must be enabled in mcuconf.h"
#endif

#if !STM32_DMA_REQUIRED
#error "STM32_DMA_REQUIRED not enabled in mcuconf.h"
#endif


#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Initialize WS2812B LED Driver.
 * 
 * @return uint8_t status code, 0 success, nonzero on error
 */
uint8_t ws2812b_led_driver_init(void);

/**
 * @brief Start WS2812B LED Driver.
 * 
 * @return uint8_t status code, 0 success, nonzero on error
 */
uint8_t ws2812b_led_driver_start(void);

/**
 * @brief Stop WS2812B LED Driver.
 * 
 * @return uint8_t status code, 0 success, nonzero on error
 */
uint8_t ws2812b_led_driver_stop(void);

/**
 * @brief Sets the color (RGB) of the WS2812B LED.
 * 
 * @return uint8_t status code, 0 success, nonzero on error
 */
uint8_t ws2812b_led_driver_set_color_rgb(uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief Renders the WS2812B LED.
 * 
 * @return uint8_t status code, 0 success, nonzero on error
 */
uint8_t ws2812b_led_driver_render(void);

/**
 * @brief Sets the color (RGB) and renders the WS2812B LED.
 * 
 * @return uint8_t status code, 0 success, nonzero on error
 */
uint8_t ws2812b_led_driver_set_color_rgb_and_render(uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief   Driver status for debugging (non-blocking query).
 * @details This struct tracks driver state and statistics without
 *          requiring blocking debug prints that would disrupt timing.
 */
typedef struct {
    uint32_t render_count;          /**< Total render calls              */
    uint32_t reset_render_count;    /**< Total reset render calls        */
    uint32_t dma_wait_count;        /**< Times waited for DMA            */
    uint8_t  last_r;                /**< Last red value set              */
    uint8_t  last_g;                /**< Last green value set            */
    uint8_t  last_b;                /**< Last blue value set             */
    bool     initialized;           /**< Driver initialized              */
    bool     started;               /**< Driver started                  */
} ws2812b_status_t;

/**
 * @brief   Get driver status for debugging.
 * @details Returns a pointer to the driver status struct. This is safe
 *          to call at any time and does not affect LED timing.
 * @return  Pointer to status struct (read-only).
 */
const ws2812b_status_t* ws2812b_led_driver_get_status(void);


#ifdef __cplusplus
}
#endif

#endif /* _WS2812B_LED_DRIVER_ */
