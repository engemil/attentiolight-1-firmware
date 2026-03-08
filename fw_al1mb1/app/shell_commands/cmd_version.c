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
 * @file    cmd_version.c
 * @brief   Shell command: version.
 *
 * @details Reads the firmware version from the application header and
 *          responds with the version string in <major>.<minor>.<patch>
 *          format followed by the OK terminator.
 *
 *          The version is encoded as a 32-bit value in the app header:
 *          bits [23:16] = major, bits [15:8] = minor, bits [7:0] = patch.
 *
 * @example
 *          -> version\r\n
 *          <- 1.1.0\r\n
 *          <- OK\r\n
 */

#include "cmd_version.h"
#include "shell_helpers.h"
#include "app_header.h"
#include "chprintf.h"

/*===========================================================================*/
/* Command handler                                                           */
/*===========================================================================*/

void cmd_version(BaseSequentialStream *chp, int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    uint32_t ver = app_header.version;
    uint8_t major = (uint8_t)((ver >> 16) & 0xFFU);
    uint8_t minor = (uint8_t)((ver >> 8) & 0xFFU);
    uint8_t patch = (uint8_t)(ver & 0xFFU);

    chprintf(chp, "%u.%u.%u\r\n", major, minor, patch);
    shell_ok(chp);
}
