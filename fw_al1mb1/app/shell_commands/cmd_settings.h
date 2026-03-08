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
 * @file    cmd_settings.h
 * @brief   Shell command: settings.
 *
 * @details Provides get/set access to persistent device settings over
 *          the shell interface. Supports reading any externally-visible
 *          field and writing fields with RW access.
 *
 *          Usage:
 *            settings get <key>          Read a setting value
 *            settings set <key> <value>  Write a setting value
 */

#ifndef CMD_SETTINGS_H
#define CMD_SETTINGS_H

#include "hal.h"

/**
 * @brief   Shell handler for the "settings" command.
 *
 * @param[in] chp   Pointer to the shell output stream.
 * @param[in] argc  Number of arguments.
 * @param[in] argv  Argument strings.
 */
void cmd_settings(BaseSequentialStream *chp, int argc, char *argv[]);

#endif /* CMD_SETTINGS_H */
