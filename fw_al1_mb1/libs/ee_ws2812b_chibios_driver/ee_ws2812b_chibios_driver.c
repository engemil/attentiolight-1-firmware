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

#include "ee_ws2812b_chibios_driver.h"

#include <stdint.h>

#define    WS2812B_T0H          400 /* Width are in nanoseconds */
#define    WS2812B_T0L          850 /* Width are in nanoseconds */
#define    WS2812B_T1H          850 /* Width are in nanoseconds */
#define    WS2812B_T1L          400 /* Width are in nanoseconds */
#define    WS2812B_T_ERROR      150 /* Width are in nanoseconds */
#define    WS2812B_RES          50000 /* Width are in nanoseconds */

// 0 code = WS2812B_T0H + WS2812B_T0L +/- WS2812B_T_ERROR = 1250 +/- 300 ns
// 1 code = WS2812B_T1H + WS2812B_T1L  +/- WS2812B_T_ERROR = 1250 +/- 300 ns
// RESET code >= WS2812B_RES = 50000 ns

typedef struct {
    uint8_t green;
    uint8_t red;
    uint8_t blue;
} ws2812b_rgb_t;

#define WS2812_LED_N    1 // Number of LEDs
#define PORT_WS2812     GPIOA
#define PIN_WS2812      1
#define WS2812_TIM_N    2  // timer, 1-11
#define WS2812_TIM_CH   1  // timer channel, 0-3
#define WS2812_DMA_STREAM STM32_DMA_STREAM_ID(1, 1)  // DMA stream for TIMx_UP (look up in reference manual under DMA Channel selection)
#define WS2812_DMA_CHANNEL 3                  // DMA channel for TIMx_UP



uint8_t ee_ws2812b_init_driver(void){
    return 0;
}

