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
 * @file    rt_config.h
 * @brief   Real-time configuration for timing-critical settings.
 *
 * @details This file centralizes thread priorities, DMA priorities, and tick
 *          rates that affect real-time behavior. Hardware IRQ priorities are
 *          configured separately in cfg/mcuconf.h.
 *
 *          Centralizing these settings makes it easier to:
 *          - Tune system timing behavior
 *          - Diagnose priority inversion issues
 *          - Document the overall RT architecture
 */

#ifndef RT_CONFIG_H
#define RT_CONFIG_H

#include "ch.h"

/*===========================================================================*/
/* Thread Priorities                                                         */
/*===========================================================================*/
/**
 * @name    Thread Priorities
 * @details ChibiOS priority range: LOWPRIO (1) to HIGHPRIO (127)
 *          NORMALPRIO = 64. Higher number = higher priority.
 * @{
 */

/**
 * @brief   Button driver thread priority.
 * @details Low priority - button processing is not timing critical.
 */
#ifndef RT_BUTTON_THREAD_PRIORITY
#define RT_BUTTON_THREAD_PRIORITY           (LOWPRIO + 1)
#endif

/**
 * @brief   State machine thread priority.
 * @details Normal priority - handles mode transitions and events.
 */
#ifndef RT_STATE_MACHINE_THREAD_PRIORITY
#define RT_STATE_MACHINE_THREAD_PRIORITY    (NORMALPRIO)
#endif

/**
 * @brief   Animation thread priority.
 * @details Above normal priority for smooth LED updates.
 *          Must be higher than state machine to prevent animation stutter.
 */
#ifndef RT_ANIMATION_THREAD_PRIORITY
#define RT_ANIMATION_THREAD_PRIORITY        (NORMALPRIO + 1)
#endif

/** @} */

/*===========================================================================*/
/* Thread Configurations                                                     */
/*===========================================================================*/

/**
 * @brief   Button thread working area size in bytes.
 */
#ifndef BTN_THREAD_WA_SIZE
#define BTN_THREAD_WA_SIZE              256
#endif

/**
 * @brief   State machine thread working area size.
 */
#ifndef APP_SM_THREAD_WA_SIZE
#define APP_SM_THREAD_WA_SIZE           512
#endif

/**
 * @brief   Animation thread working area size.
 */
#ifndef APP_SM_ANIM_THREAD_WA_SIZE
#define APP_SM_ANIM_THREAD_WA_SIZE      512
#endif

/**
 * @brief   Input event queue size.
 */
#ifndef APP_SM_INPUT_QUEUE_SIZE
#define APP_SM_INPUT_QUEUE_SIZE         8
#endif

/** @} */

/*===========================================================================*/
/* DMA Priorities                                                            */
/*===========================================================================*/
/**
 * @name    DMA Priorities
 * @details STM32 DMA priority levels: 0 (low) to 3 (very high).
 *          Higher priority DMA requests are serviced first.
 * @{
 */

/**
 * @brief   WS2812B LED DMA priority.
 * @details Medium-high priority (2) to prevent timing corruption from
 *          other DMA or interrupt activity. WS2812B protocol has strict
 *          timing requirements (~0.4us tolerance).
 *
 * @note    If white flashes occur, consider increasing to 3 (very high).
 */
#ifndef RT_WS2812B_DMA_PRIORITY
#define RT_WS2812B_DMA_PRIORITY             2
#endif

/** @} */

/*===========================================================================*/
/* Tick Rates / Update Intervals                                             */
/*===========================================================================*/
/**
 * @name    Tick Rates
 * @details Periodic update intervals for various subsystems.
 * @{
 */

/**
 * @brief   Animation update interval in milliseconds.
 * @details 33ms = ~30 FPS. Good balance of smoothness and CPU usage.
 *          Lower values (e.g., 20ms = 50 FPS) increase smoothness but
 *          also increase CPU load and DMA contention risk.
 */
#ifndef RT_ANIMATION_TICK_MS
#define RT_ANIMATION_TICK_MS                33
#endif

/**
 * @brief   Button poll interval in milliseconds.
 * @details How often the button thread checks for state changes.
 *          50ms is adequate for human interaction.
 */
#ifndef RT_BUTTON_POLL_MS
#define RT_BUTTON_POLL_MS                   50
#endif

/** @} */

/*===========================================================================*/
/* Reference: Hardware IRQ Priorities (cfg/mcuconf.h)                        */
/*===========================================================================*/
/**
 * @name    IRQ Priority Reference
 * @details Hardware interrupt priorities are configured in cfg/mcuconf.h.
 *          STM32C0 uses 2-bit priority (0-3), where 0 is highest.
 *
 *          Current assignments:
 *          | Peripheral      | Priority | Notes                           |
 *          |-----------------|----------|----------------------------------|
 *          | TIM1 (WS2812B)  | 1        | High - timing critical PWM      |
 *          | TIM16 (systick) | 2        | Medium - OS tick                |
 *          | USART1/2        | 2        | Medium - serial debug           |
 *          | EXTI (buttons)  | 3        | Low - not timing critical       |
 *          | USB             | 3        | Low - bulk transfers            |
 *          | I2C1            | 3        | Low - not currently used        |
 *
 * @note    Do not duplicate these here - edit cfg/mcuconf.h directly.
 * @{
 */
/** @} */

#endif /* RT_CONFIG_H */
