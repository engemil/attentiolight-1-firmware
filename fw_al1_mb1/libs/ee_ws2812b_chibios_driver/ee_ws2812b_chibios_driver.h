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

#include "ch.h"
#include "hal.h"

#if !HAL_USE_PWM
#error "PWM not enabled in halconf.h"
#endif

#ifdef __cplusplus
extern "C"
{
#endif


/*
typedef struct {
    ee_ws2812b_config_t *config;
    uint8_t dummy;
} ee_ws2812b_driver_t;

typedef struct {
    uint8_t green;
    uint8_t red;
    uint8_t blue;
} ee_ws2812b_config_t;
*/

//static ee_ws2812b_config_t ee_ws2812b_default_config = {
//    .green = 0;
//    .red = 0;
//    .blue = 0;
//};


/**
 * @brief Initialize EngEmil PMW3901MB Driver.
 * 
 * @return uint8_t status code, 0 success, nonzero on error
 */
uint8_t ee_ws2812b_init_driver(void);




#ifdef __cplusplus
}
#endif

#endif /* _EE_WS2812B_CHIBIOS_DRIVER_ */