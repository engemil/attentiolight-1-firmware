# Firmware AttentioLight1-MainBoard1

Hardware
- Microcontroller: STM32C071RB


Firmware stack
- ChibiOS
- more..


(IN-PROGRESS)




## Addiitonal Info

- Info from https://github.com/cbiffle/stm32c0-metapac-example
    - STM32C0 support was added to OpenOCD after the 0.12 release, so you will need it built from git. I used the openocd-git AUR package on Arch. If you've used OpenOCD for any length of time, you'll be accustomed to having to build it from git. :-)
    - The C0 series picked up the same odd behavior from the G0 series, where the EMPTY bit in the flash controller -- used to determine if there's code worth booting in the flash -- seems not to get re-evaluated at reset, and only at power-on. This means the very first time you program an STM32C0, if you reset it, it will bounce right back into the ROM like it's not programmed. To fix this, power cycle it. This is only necessary when starting with a factory-fresh part.
    - Remember to mux your dual-function pins correctly in SYSCFG.
- For float support, remember to set (`CHPRINTF_USE_FLOAT`) to TRUE in `ChibiOS/os/hal/lib/streams/chprintf.h`


