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
 * @file    state_off.c
 * @brief   Off state implementation with low-power Stop mode.
 *
 * @details The off state puts the MCU into STM32 Stop mode to minimize power
 *          consumption while waiting for user input to wake up.
 *
 *          Stop mode characteristics:
 *          - All clocks stopped (except LSE/LSI)
 *          - RAM and register contents retained
 *          - Wake-up via EXTI (button)
 *          - Typical current: ~2.4µA
 *
 *          Wake-up sources:
 *          - Button press (PC11 via EXTI)
 *          - USB activity (TODO: not yet implemented)
 *
 * @note    Stop mode can be disabled for debugging by building with
 *          `make debug` which defines APP_DEBUG_BUILD=1. This keeps the
 *          debugger connected but increases power consumption.
 */

#include "system_states.h"
#include "animation_thread.h"
#include "button_driver.h"
#include "app_log.h"
#include "portab.h"
#include "usbcfg.h"

/*===========================================================================*/
/* Low-Power Mode Configuration                                              */
/*===========================================================================*/

/**
 * @brief   Button wake-up line (PC11 = EXTI line 11).
 */
#define WAKEUP_BUTTON_LINE          LINE_USER_BUTTON
#define WAKEUP_BUTTON_EXTI_LINE     PAL_PAD(WAKEUP_BUTTON_LINE)

/*===========================================================================*/
/* Low-Power Mode Helper Functions                                           */
/*===========================================================================*/

#if !defined(APP_DEBUG_BUILD) || (APP_DEBUG_BUILD != 1)

/**
 * @brief   Prepares the system for entering Stop mode.
 * @details Stops peripherals and configures wake-up sources.
 *
 * @note    Called before entering Stop mode. Peripherals are stopped to
 *          prevent spurious interrupts and reduce power consumption.
 */
static void prepare_stop_mode(void) {
    /*
     * Stop USB peripheral.
     * USB will be restarted after wake-up.
     */
    usbStop(serusbcfg1.usbp);

    /*
     * Configure button EXTI as wake-up source.
     * The button driver already configured the EXTI line, but we ensure
     * the interrupt is enabled and pending flags are cleared.
     */
    
    /* Clear any pending EXTI flags for button line */
    EXTI->RPR1 = (1U << WAKEUP_BUTTON_EXTI_LINE);  /* Clear rising edge pending */
    EXTI->FPR1 = (1U << WAKEUP_BUTTON_EXTI_LINE);  /* Clear falling edge pending */

    /*
     * TODO: USB wake-up from Stop mode.
     * Currently not implemented. Possible approaches:
     * 1. Keep USB in suspend mode (don't call usbStop) so USB_CNTR_WKUPM works
     * 2. Configure USB D+ pin (PA12) as GPIO EXTI wake-up source
     * 3. Use USB VBUS detection if available on hardware
     */

    /*
     * Ensure PWR wake-up is enabled.
     * PWR_CR3_EIWUL should already be set in mcuconf.h, but verify here.
     */
    PWR->CR3 |= PWR_CR3_EIWUL;          /* Enable external interrupt wake-up */

    /*
     * Clear PWR wake-up flags.
     */
    PWR->SCR = PWR_SCR_CWUF;            /* Clear all wake-up flags */
}

/**
 * @brief   Enters Stop mode.
 * @details Configures the MCU for Stop mode and executes WFI.
 *          The CPU halts until a wake-up event occurs.
 *
 * @note    SysTick is disabled before entering Stop mode to prevent
 *          the scheduler from running. This is safe because no useful
 *          work happens in the off state.
 */
static void enter_stop_mode(void) {
    /*
     * Disable SysTick to prevent spurious wake-ups.
     * ChibiOS scheduler will be suspended during Stop mode.
     */
    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;

    /*
     * Configure Stop mode in PWR peripheral.
     * LPMS[2:0] = 000 for Stop 0 mode (main regulator on)
     * LPMS[2:0] = 001 for Stop 1 mode (low-power regulator) - lower power
     * We use Stop 1 for lowest power with RAM retention.
     */
    PWR->CR1 = (PWR->CR1 & ~PWR_CR1_LPMS_Msk) | PWR_CR1_LPMS_0;

    /*
     * Set SLEEPDEEP bit in Cortex-M0+ System Control Register.
     * This is required for Stop mode; without it, WFI enters Sleep mode.
     */
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;

    /*
     * Data Synchronization Barrier.
     * Ensures all memory accesses are complete before entering Stop mode.
     */
    __DSB();

    /*
     * Wait For Interrupt.
     * CPU halts here until a wake-up event (EXTI) occurs.
     */
    __WFI();

    /*
     * --- Wake-up occurred, execution continues here ---
     */

    /*
     * Clear SLEEPDEEP bit.
     * This ensures subsequent WFI instructions enter Sleep mode, not Stop.
     */
    SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;
}

/**
 * @brief   Restores the system after waking from Stop mode.
 * @details Re-initializes clocks, restarts peripherals, and clears flags.
 *
 * @note    After Stop mode, the system clock is HSI. High-speed clocks
 *          (HSI48 for USB) need to be re-enabled.
 */
static void restore_from_stop(void) {
    /*
     * Re-enable SysTick.
     * ChibiOS scheduler resumes operation.
     */
    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;

    /*
     * Re-enable HSI48 for USB.
     * After Stop mode, HSI48 is disabled and needs to be restarted.
     * On STM32C0, this is called HSIUSB48 (HSI USB 48MHz oscillator).
     */
    RCC->CR |= RCC_CR_HSIUSB48ON;
    while ((RCC->CR & RCC_CR_HSIUSB48RDY) == 0U) {
        /* Wait for HSI48 to stabilize */
    }

    /*
     * Clear wake-up flags.
     */
    PWR->SCR = PWR_SCR_CWUF;            /* Clear all wake-up flags */
    
    /* Clear EXTI pending flags for button */
    EXTI->RPR1 = (1U << WAKEUP_BUTTON_EXTI_LINE);
    EXTI->FPR1 = (1U << WAKEUP_BUTTON_EXTI_LINE);

    /*
     * Restart USB peripheral.
     * This re-enables USB communication after wake-up.
     */
    usbStart(serusbcfg1.usbp, &usbcfg);
    usbConnectBus(serusbcfg1.usbp);
}

#endif /* !APP_DEBUG_BUILD */

/*===========================================================================*/
/* Off State Implementation                                                  */
/*===========================================================================*/

void state_off_enter(void) {
    /* Activate button driver to allow wake-up */
    button_start();

    /* Ensure LEDs are off */
    anim_thread_off();

#if !defined(APP_DEBUG_BUILD) || (APP_DEBUG_BUILD != 1)
    LOG_DEBUG("Entering Stop mode (low-power)...");
    
    /* Prepare peripherals for Stop mode */
    prepare_stop_mode();

    /* Enter Stop mode - CPU halts until wake-up */
    enter_stop_mode();

    /* Wake-up occurred - restore system */
    restore_from_stop();

    LOG_DEBUG("Woke up from Stop mode");
#else
    LOG_DEBUG("Stop mode disabled (debug build), staying in run mode");
#endif
}

void state_off_process(app_sm_input_t input) {
    (void)input;
    /* Nothing, ignore all inputs */
}

void state_off_exit(void) {
    /* Nothing special needed - system already restored in state_off_enter() */
}
