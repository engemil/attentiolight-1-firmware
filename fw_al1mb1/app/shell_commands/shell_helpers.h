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
 * @file    shell_helpers.h
 * @brief   Response helper macros for shell command handlers.
 *
 * @details Provides consistent response formatting for the shell command
 *          protocol. All commands must terminate their response with either
 *          shell_ok() or shell_error(). Optional payload lines can be sent
 *          with shell_value() before the terminator.
 *
 *          Protocol format (see shell_commands.md):
 *          - Success: [payload lines...] OK\r\n
 *          - Failure: ERROR <message>\r\n
 */

#ifndef SHELL_HELPERS_H
#define SHELL_HELPERS_H

#include "chprintf.h"

/**
 * @brief   Send success terminator.
 * @details Writes "OK\r\n" to the shell stream. Must be the last output
 *          from a successful command handler.
 *
 * @param[in] chp   Pointer to the shell output stream.
 */
#define shell_ok(chp) \
    chprintf(chp, "OK\r\n")

/**
 * @brief   Send error terminator with message.
 * @details Writes "ERROR <msg>\r\n" to the shell stream. Must be the last
 *          (and typically only) output from a failed command handler.
 *
 * @param[in] chp   Pointer to the shell output stream.
 * @param[in] msg   Human-readable error message string.
 */
#define shell_error(chp, msg) \
    chprintf(chp, "ERROR %s\r\n", msg)

/**
 * @brief   Send a payload value line.
 * @details Writes "<val>\r\n" to the shell stream. Used to send result data
 *          before the OK terminator. Can be called multiple times for
 *          multi-line responses.
 *
 * @param[in] chp   Pointer to the shell output stream.
 * @param[in] val   Value string to send.
 */
#define shell_value(chp, val) \
    chprintf(chp, "%s\r\n", val)

#endif /* SHELL_HELPERS_H */
