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
 * @file    cmd_dfu.c
 * @brief   Shell command: dfu.
 *
 * @details Writes the bootloader magic value (0xDEADBEEF) to the last 4
 *          bytes of RAM (0x20005FFC) and triggers an NVIC system reset.
 *          The bootloader checks this address on startup and enters DFU
 *          mode when the magic value is present.
 *
 *          No response is sent — the device reboots immediately and the
 *          USB connection drops. The host CLI detects the disconnection
 *          and proceeds with the DFU flashing flow.
 *
 * @example
 *          -> dfu\r\n
 *             (device disconnects and re-enumerates in DFU mode)
 */

#include "cmd_dfu.h"

/*===========================================================================*/
/* Bootloader magic constants                                                */
/*===========================================================================*/

/**
 * @brief   RAM address for bootloader magic value.
 * @details Last 4 bytes of 24KB RAM: 0x20000000 + 0x6000 - 4 = 0x20005FFC.
 *          Must match the bootloader's BOOTLOADER_MAGIC_ADDR.
 */
#define DFU_MAGIC_ADDR      ((volatile uint32_t *)0x20005FFCU)

/**
 * @brief   Magic value that triggers DFU mode on next boot.
 * @details Must match the bootloader's BOOTLOADER_MAGIC.
 */
#define DFU_MAGIC_VALUE     0xDEADBEEFU

/*===========================================================================*/
/* Command handler                                                           */
/*===========================================================================*/

void cmd_dfu(BaseSequentialStream *chp, int argc, char *argv[]) {
    (void)chp;
    (void)argc;
    (void)argv;

    /* Write magic value to RAM — survives software reset. */
    *DFU_MAGIC_ADDR = DFU_MAGIC_VALUE;

    /* Trigger system reset. The bootloader will detect the magic value
     * and enter DFU mode instead of jumping to the application. */
    NVIC_SystemReset();

    /* Unreachable — NVIC_SystemReset() never returns. */
}
