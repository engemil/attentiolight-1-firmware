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
 * @file    cmd_metadata.h
 * @brief   Shell command: metadata.
 *
 * @details Returns read-only device metadata: serial number, firmware version,
 *          device model, hardware revision, build date, and chip UID.
 *          All values are gathered from their native sources (EFL, app header,
 *          compile-time macros, silicon registers).
 */

#ifndef CMD_METADATA_H
#define CMD_METADATA_H

#include "hal.h"

/**
 * @brief   Shell handler for the "metadata" command.
 *
 * @param[in] chp   Pointer to the shell output stream.
 * @param[in] argc  Number of arguments.
 * @param[in] argv  Argument strings.
 */
void cmd_metadata(BaseSequentialStream *chp, int argc, char *argv[]);

#endif /* CMD_METADATA_H */
