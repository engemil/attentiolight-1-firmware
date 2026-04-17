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

/* TO DO: Add checks for already initialized */
/* TO DO: Add error handling */
/* TO DO: Add parameter for pin selection, timer selection, etc. */
/* TO DO: Add function to set PWM driver depending on timer selection */
/* TO DO: Add a check if it's already freed/released before calling dmaStreamFree() */
/* TO DO: Add functionality to control the power to the LED (on/off) (e.g., via a GPIO pin) */

/**
 * @file ws2812b_led_driver.c
 * 
 * @brief WS2812B LED Driver.
 * 
 */

#include "ws2812b_led_driver.h"
#include "app_log.h"
#include "rt_config.h"

/* Driver status tracking (non-blocking, safe to query anytime) */
static ws2812b_status_t driver_status = {0};


#define PWM_HI (14)
#define PWM_LO (6)
#define BITS_PER_PIXEL (24)
#define PWM_BUFFER_SIZE (BITS_PER_PIXEL + 1U) // Include 1 extra bit
#define PWM_RESET_BUFFER_SIZE (40U) // 40 * 1.25us = 50us reset time
//#define PWM_DRIVER (&PWMD16)
#define PWM_DRIVER (&PWMD1)

#define DMA_DRIVER (1U) // DMA1
#define DMA_CHANNEL (1U) // Channel 1
//#define DMA_REQUEST (44U) // DMAMUX request 44 for TIM16_CH1 (See RM0490 Reference Manual, Table 49)
//#define DMA_PERIPHERAL (&(TIM16->CCR1)) // DMA peripheral address
#define DMA_REQUEST (22U) // DMAMUX request 22 for TIM1_CH3 (See RM0490 Reference Manual, Table 49)
#define DMA_PERIPHERAL (&(TIM1->CCR3)) // DMA peripheral address

// Common DMA mode settings (with MINC)
#define DMA_MODE_1 ( \
    STM32_DMA_CR_DIR_M2P /* Memory to peripheral */ \
    | STM32_DMA_CR_MINC /* Increment memory pointer */ \
    | STM32_DMA_CR_PSIZE_HWORD /* Peripheral size 16 bits */ \
    | STM32_DMA_CR_MSIZE_BYTE /* Memory size 8 bits */ \
    | STM32_DMA_CR_TCIE /* Transfer complete interrupt enable */ \
    | STM32_DMA_CR_TEIE /* Transfer error interrupt enable */ \
    | STM32_DMA_CR_PL(RT_WS2812B_DMA_PRIORITY) /* Priority from rt_config.h */ \
)

// Common DMA mode settings (without MINC)
#define DMA_MODE_2 ( \
    STM32_DMA_CR_DIR_M2P /* Memory to peripheral */ \
    | STM32_DMA_CR_PSIZE_HWORD /* Peripheral size 16 bits */ \
    | STM32_DMA_CR_MSIZE_BYTE /* Memory size 8 bits */ \
    | STM32_DMA_CR_TCIE /* Transfer complete interrupt enable */ \
    | STM32_DMA_CR_TEIE /* Transfer error interrupt enable */ \
    | STM32_DMA_CR_PL(RT_WS2812B_DMA_PRIORITY) /* Priority from rt_config.h */ \
)


const uint8_t pwm_zero_buf = 0;
uint8_t pwm_buf[PWM_BUFFER_SIZE] = {0}; // Ensure last value is untouched (always zero).
static const stm32_dma_stream_t *dma_stream; //= NULL; // Global DMA stream pointer
static volatile bool dma_ready = true;
//static volatile bool driver_started = false; // Track if driver already started


static const PWMConfig pwm_cfg = {
    .frequency  = 16000000,  // Counter clock frequency for PSC=2
    .period     = 20,        // PWM period in ticks (ARR + 1)
    .callback   = NULL,
    .channels   = {
        {.mode  = PWM_OUTPUT_DISABLED, .callback = NULL},
        {.mode  = PWM_OUTPUT_DISABLED,    .callback = NULL},
        {.mode  = PWM_OUTPUT_ACTIVE_HIGH,    .callback = NULL},
        {.mode  = PWM_OUTPUT_DISABLED,    .callback = NULL}
    },
    .cr2        = STM32_TIM_CR2_CCDS,  // DMA requests on capture/compare events
#if STM32_ADVANCED_DMA
    .bdtr       = 0,
#endif
    .dier       = STM32_TIM_DIER_CC3DE  // Enable DMA on CC3 event (TIMx_CH3 / PWM Channel 3)
};

static void dma_callback(void *p, uint32_t flags) {
    (void)p;
    if (flags & STM32_DMA_ISR_TCIF) {
        dma_ready = true;
    }
    if (flags & STM32_DMA_ISR_TEIF) {
        /* DMA transfer error - count it and allow retry on next frame */
        driver_status.dma_error_count++;
        dma_ready = true;
    }
}

uint8_t ws2812b_led_driver_init(void){
    LOG_DEBUG("WS2812B: init");
    driver_status.initialized = true;
    ws2812b_led_driver_start();
    return 0;
}

uint8_t ws2812b_led_driver_start(void){
    LOG_DEBUG("WS2812B: start");

    /* Prevent double initialization */
    if (driver_status.started) {
        LOG_WARN("WS2812B: start skipped - already started");
        return 0;
    }

    // Start PWM
    pwmStart(PWM_DRIVER, &pwm_cfg);

    // Setup DMA
    dma_stream = dmaStreamAlloc(STM32_DMA_STREAM_ID(DMA_DRIVER, DMA_CHANNEL),
                                    RT_WS2812B_DMA_PRIORITY, (stm32_dmaisr_t)dma_callback, NULL);
    if (dma_stream == NULL) {
        LOG_ERROR("WS2812B: DMA alloc failed!");
        return 1;
    }

    dmaSetRequestSource(dma_stream, DMA_REQUEST);
    dmaStreamSetPeripheral(dma_stream, DMA_PERIPHERAL);

    driver_status.started = true;
    LOG_DEBUG("WS2812B: started OK");
    return 0;
}

uint8_t ws2812b_led_driver_stop(void){
    LOG_DEBUG("WS2812B: stop");

    if (!driver_status.started) {
        LOG_DEBUG("WS2812B: stop skipped - not started");
        return 0;
    }

    dmaStreamDisable(dma_stream);
    dmaStreamFree(dma_stream);
    dma_stream = NULL;
    pwmStop(PWM_DRIVER);

    driver_status.started = false;
    return 0;
}

uint8_t ws2812b_led_driver_set_color_rgb(uint8_t r, uint8_t g, uint8_t b) {
    /* Track last color (no serial prints - timing critical) */
    driver_status.last_r = r;
    driver_status.last_g = g;
    driver_status.last_b = b;

    // Send bits MSB-first (bit 7 first, bit 0 last) as required by WS2812B protocol
    for(int i = 0; i < 8; i++){
        pwm_buf[i] = (g & (1 << (7 - i))) ? PWM_HI : PWM_LO;
        pwm_buf[i + 8] = (r & (1 << (7 - i))) ? PWM_HI : PWM_LO;
        pwm_buf[i + 16] = (b & (1 << (7 - i))) ? PWM_HI : PWM_LO;
    }
    // pwm_buf[24] remains 0

    return 0;
}

uint8_t ws2812b_led_driver_reset_render(void){

    while (!dma_ready) {
        driver_status.dma_wait_count++;
        chThdSleepMilliseconds(1);  // Wait for previous DMA to complete
    }
    dma_ready = false;

    // Send reset by sending low signal for at least 50us

    dmaStreamDisable(dma_stream);
    dmaStreamSetMode(dma_stream, DMA_MODE_2); // No MINC for repeated zero
    dmaStreamSetMemory0(dma_stream, &pwm_zero_buf);
    dmaStreamSetTransactionSize(dma_stream, PWM_RESET_BUFFER_SIZE);
    dmaStreamEnable(dma_stream);

    driver_status.reset_render_count++;
    return 0;
}

uint8_t ws2812b_led_driver_render(void) {
    uint8_t status = 0;

    // First reset the render
    status = ws2812b_led_driver_reset_render();

    while (!dma_ready) {
        driver_status.dma_wait_count++;
        chThdSleepMilliseconds(1);  // Wait for previous DMA to complete
    }
    dma_ready = false;


    // Write the actual data

    dmaStreamDisable(dma_stream);
    dmaStreamSetMode(dma_stream, DMA_MODE_1);
    dmaStreamSetMemory0(dma_stream, pwm_buf);
    dmaStreamSetTransactionSize(dma_stream, PWM_BUFFER_SIZE);
    dmaStreamEnable(dma_stream);

    driver_status.render_count++;
    return status;
}

uint8_t ws2812b_led_driver_set_color_rgb_and_render(uint8_t r, uint8_t g, uint8_t b){
    ws2812b_led_driver_set_color_rgb(r, g, b);
    ws2812b_led_driver_render();

    return 0;
}

const ws2812b_status_t* ws2812b_led_driver_get_status(void) {
    return &driver_status;
}
