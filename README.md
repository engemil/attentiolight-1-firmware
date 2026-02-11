# AttentioLight-1 MainBoard-1 Firmware

(DEVELOPMENT IN-PROGRESS)

This is the source code (firmware) for the AttentioLight-1 MainBoard-1 (`al1mb1`).

(TO DO: Add more info)


## Dependencies

### Required Tools
- **arm-none-eabi-gcc** (13.2.1 or later) - ARM cross-compiler.
- **make** (4.3 or later) - Build system.
- **st-flash** or **openocd** - Hardware programming.
  - NB! **stlink** v1.8.0 does not have STM32C071xx included. On Ubuntu 24.04 MUST build from source with `develop`-branch [link](https://github.com/stlink-org/stlink).
  - NB! **openocd** v0.12.0 or newer. On Ubuntu 24.04 you MUST build from source [link](https://github.com/openocd-org/openocd).
- **dfu-util** (0.11 or later) - USB Device Firmware Upgrade (DFU). Firmware uploads over USB.


### Installation (Ubuntu/Debian)
```bash
sudo apt update
sudo apt install git build-essential gcc-arm-none-eabi \
  binutils-arm-none-eabi libnewlib-arm-none-eabi \
  python3 dfu-util -y

# NB! We also need stlink-tools and openocd, but they are outdated on ubuntu 24.04.
# see the .devcontainer/Dockerfile for how to install from source, or use docker container
#sudo apt install stlink-tools openocd -y
```

### Setup Repository

```bash
git clone https://github.com/engemil/ee_stm32_bootloader.git
cd ee_stm32_bootloader
git submodule update --init --recursive
```

See `ext/README.md` for detailed submodule management.


## Quick Start

**(If not already done) upload Bootloader Firmware via ST-Link (probe):**

```bash
# Build bootloader
cd bootloader && make clean && make
# Flash to device over debugger (the first stlink it sees)
st-flash --reset write build/bootloader.bin 0x08000000
# Go back to project root folder
cd ..
# Verify successful flash of bootloader to see if device has entered USB DFU mode
sudo apt install usbutils -y
lsusb
```

**Upload Application Firmware via USB DFU (with Bootloader installed):**
```bash

# Upload application firmware over USB
cd fw_al1mb1 && make clean && make

# Upload test firmware
sudo dfu-util -a 0 --dfuse-address 0x08004000:leave -D build/fw_al1mb1_signed.bin
```


## Project Structure

(TO DO: To be added)


## Hardware

- Microcontroller: STM32C071RB

(TO DO: Add more info)


## Firmware stack

- ChibiOS
- more..

(TO DO: Add more info)


## Libraries and Drivers

- usbcfg and portab (temporary USB related files)

(TO DO move to cfg file and a portability/drivers/library folder?)

**Project-based Libraries/Drivers**
- EngEmil WS2812B ChibiOS Driver (application driver) 


## Bugs and Issues


### Fixing chThdSleep on STM32C071RB

- Changes in projects files:
    - `cfg/mcuconf.h`
        ```
        #define STM32_ST_USE_TIMER                  17 // TIM17. Note that TIM2 is not working with ChibiOS on STM32C071RB
        ```
    - `cfg/chconf.h`
        ```
        #define CH_CFG_ST_RESOLUTION                16 // Adjust for 16-bit timer. Default was 32-bit.
        #define CH_CFG_INTERVALS_SIZE               16 // Adjust for 16-bit timer. Default was 32-bit.
        #define CH_CFG_TIME_TYPES_SIZE              16 // Adjust for 16-bit timer. Default was 32-bit.
        ```


### Fixing Serial over USB on STM32C071RB

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

- Add temporary missing USB port for STM32C0xx
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



### First time programming a STM32C071RB

(Info from https://github.com/cbiffle/stm32c0-metapac-example)

The STM32C0xx series picked up the same odd behavior from the STM32G0xx series, where the EMPTY bit in the flash controller (used to determine if there's code worth booting in the flash) seems not to get re-evaluated at reset, and only at power-on. This means the very first time you program an STM32C0xx, if you reset it, it will bounce right back into the ROM like it's not programmed.

To fix this, power cycle it. This is only necessary when starting with a factory-fresh part.


## Test (TO DO: Remove later)

Serial Communication, both VCP (through STLINK) and USB.

- `lsusb`
- `ls /dev/ttyACM* /dev/ttyUSB*`
- `minicom -D /dev/ttyACM0 -b 115200`
- `minicom -D /dev/ttyACM1 -b 115200`



## License

MIT License, see `LICENSE`-file for details.

Portions of this project incorporate code from:
- **ChibiOS**, Apache License 2.0.
- **STMicroelectronics**, Apache License 2.0.
- **ee_stm32_bootloader**, MIT License.
- **ee_ws2812b_chibios_driver**, MIT License.
- **ee_esp32_wifi_ble_if_driver**, MIT License.

For submodule lincenses, see individual repository LICENSE files for details.
