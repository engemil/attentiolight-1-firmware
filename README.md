# AttentioLight-1 MainBoard1 Firmware

Hardware
- Microcontroller: STM32C071RB


Firmware stack
- ChibiOS
- more..


(IN-PROGRESS)



## Fixing chThdSleep on STM32C071RB

- Changes in projects files:
    - `cfg/mcuconf.h`
        ```
        #define STM32_ST_USE_TIMER                  17 // TIM17
        ```
    - `cfg/chconf.h`
        ```
        #define CH_CFG_ST_RESOLUTION                16
        #define CH_CFG_INTERVALS_SIZE               16
        #define CH_CFG_TIME_TYPES_SIZE              16
        ```


## Fixing Serial over USB on STM32C071RB

- Changes in projects files for Serial over USB
    - `cfg/halconf.h`
        ```
        #define HAL_USE_SERIAL TRUE
        ...
        #define HAL_USE_SERIAL_USB TRUE
        ...
        #define HAL_USE_USB TRUE
        ```
    - `cfg/mcuconf.h`
        ```
        #define STM32_USB_USE_USB1 TRUE
        ...
        #define STM32_USB_USB1_LP_IRQ_PRIORITY      3
        ```
    - `Makefile`
        ```
        include $(CHIBIOS)/os/hal/lib/streams/streams.mk
        ```

- Temporary missing USB port for STM32C0xx
    - In `os/hal/ports/STM32/STM32C0xx/stm32_isr.h` (line 129 to 137), add the following
        ```
        /*
        * USB units.
        */
        #define STM32_USB1_HP_HANDLER               Vector60
        #define STM32_USB1_LP_HANDLER               Vector60
        #define STM32_USB1_HP_NUMBER                8
        #define STM32_USB1_LP_NUMBER                8
        ```

    - In `os/hal/ports/STM32/STM32C0xx/stm32_rcc.h` (line 611 to 666), add the following
        ```
        /**
        * @name    USB peripheral specific RCC operations
        * @{
        */
        /**
        * @brief   Enables the USB peripheral clock.
        *
        * @param[in] lp        low power enable flag
        *
        * @api
        */
        #define rccEnableUSB(lp) rccEnableAPBR1(RCC_APBENR1_USBEN, lp)

        /**
        * @brief   Disables the USB peripheral clock.
        *
        * @api
        */
        #define rccDisableUSB() rccDisableAPBR1(RCC_APBENR1_USBEN)

        /**
        * @brief   Resets the USB peripheral.
        *
        * @api
        */
        #define rccResetUSB() rccResetAPBR1(RCC_APBRSTR1_USBRST)
        /** @} */

        /**
        * @name    CRC peripheral specific RCC operations
        * @{
        */
        /**
        * @brief   Enables the CRC peripheral clock.
        *
        * @param[in] lp        low power enable flag
        *
        * @api
        */
        #define rccEnableCRC(lp) rccEnableAHB(RCC_AHBENR_CRCEN, lp)

        /**
        * @brief   Disables the CRC peripheral clock.
        *
        * @api
        */
        #define rccDisableCRC() rccDisableAHB(RCC_AHBENR_CRCEN)

        /**
        * @brief   Resets the CRC peripheral.
        *
        * @api
        */
        #define rccResetCRC() rccResetAHB(RCC_AHBRSTR_CRCRST)
        /** @} */
        ```


## Additonal Info

- Info from https://github.com/cbiffle/stm32c0-metapac-example
    - STM32C0 support was added to OpenOCD after the 0.12 release, so you will need it built from git. I used the openocd-git AUR package on Arch. If you've used OpenOCD for any length of time, you'll be accustomed to having to build it from git. :-)
    - The C0 series picked up the same odd behavior from the G0 series, where the EMPTY bit in the flash controller -- used to determine if there's code worth booting in the flash -- seems not to get re-evaluated at reset, and only at power-on. This means the very first time you program an STM32C0, if you reset it, it will bounce right back into the ROM like it's not programmed. To fix this, power cycle it. This is only necessary when starting with a factory-fresh part.
    - Remember to mux your dual-function pins correctly in SYSCFG.
- For float support, remember to set (`CHPRINTF_USE_FLOAT`) to TRUE in `ChibiOS/os/hal/lib/streams/chprintf.h`


