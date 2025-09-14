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
 * @file ee_ws2812b_chibios_driver.h
 * 
 * @brief EngEmil WS2812B ChibiOS Driver.
 * 
 */

#ifndef _EE_WS2812B_CHIBIOS_DRIVER_
#define _EE_WS2812B_CHIBIOS_DRIVER_

#include <stdint.h>
#include <string.h>

#include "ch.h"
#include "hal.h"

#ifndef STM32C011xx
#ifndef STM32C031xx
#ifndef STM32C051xx
#ifndef STM32C071xx
#ifndef STM32C091xx
#ifndef STM32C092xx
#error "EngEmil WS2812B ChibiOS Driver only supports STM32C0 series"
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
 * @brief Initialize EngEmil PMW3901MB Driver.
 * 
 * @return uint8_t status code, 0 success, nonzero on error
 */
uint8_t ee_ws2812b_init_driver(void);

/**
 * @brief Start EngEmil PMW3901MB Driver.
 * 
 * @return uint8_t status code, 0 success, nonzero on error
 */
uint8_t ee_ws2812b_start_driver(void);

/**
 * @brief Stop EngEmil PMW3901MB Driver.
 * 
 * @return uint8_t status code, 0 success, nonzero on error
 */
uint8_t ee_ws2812b_stop_driver(void);

/**
 * @brief Sets the color (RGB) of the WS2812B LED.
 * 
 * @return uint8_t status code, 0 success, nonzero on error
 */
uint8_t ee_ws2812b_set_color_rgb(uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief Renders the WS2812B LED.
 * 
 * @return uint8_t status code, 0 success, nonzero on error
 */
uint8_t ee_ws2812b_render(void);

/**
 * @brief Sets the color (RGB) and renders the WS2812B LED.
 * 
 * @return uint8_t status code, 0 success, nonzero on error
 */
uint8_t ee_ws2812b_set_color_rgb_and_render(uint8_t r, uint8_t g, uint8_t b);


#ifdef __cplusplus
}
#endif

#endif /* _EE_WS2812B_CHIBIOS_DRIVER_ */