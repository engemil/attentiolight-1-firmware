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
 * @file    cmd_dfu.h
 * @brief   Shell command: dfu.
 *
 * @details Triggers a reboot into DFU bootloader mode by writing a magic
 *          value to RAM and performing a system reset. The device disconnects
 *          immediately with no response — the host detects the USB
 *          disconnection and proceeds with DFU flashing.
 */

#ifndef CMD_DFU_H
#define CMD_DFU_H

#include "hal.h"

/**
 * @brief   Shell handler for the "dfu" command.
 *
 * @param[in] chp   Pointer to the shell output stream (unused — no response).
 * @param[in] argc  Number of arguments (unused).
 * @param[in] argv  Argument strings (unused).
 */
void cmd_dfu(BaseSequentialStream *chp, int argc, char *argv[]);

#endif /* CMD_DFU_H */
