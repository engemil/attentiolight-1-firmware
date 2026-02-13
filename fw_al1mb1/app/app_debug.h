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
 * @file    app_debug.h
 * @brief   Centralized debug configuration and macros.
 *
 * @details This header provides a unified debug system for the application.
 *          Debug levels can be configured at compile time via Makefile:
 *
 *          make                  - Release build (no debug output)
 *          make debug            - Debug with INFO level (default LEVEL=3)
 *          make debug LEVEL=1    - Errors only
 *          make debug LEVEL=2    - Errors + warnings
 *          make debug LEVEL=3    - Errors + warnings + info
 *          make debug LEVEL=4    - Everything (verbose)
 *
 *          Debug levels are hierarchical:
 *            0 = NONE  - No output (release build)
 *            1 = ERROR - Critical errors only
 *            2 = WARN  - Errors + warnings
 *            3 = INFO  - Errors + warnings + state/mode changes
 *            4 = DEBUG - Everything (verbose details)
 */

#ifndef APP_DEBUG_H
#define APP_DEBUG_H

#include "ch.h"
#include "hal.h"
#include "chprintf.h"

/* Forward declaration - SDU1 is defined in usbcfg.c */
extern SerialUSBDriver SDU1;

/*===========================================================================*/
/* Debug Level Configuration                                                 */
/*===========================================================================*/

/**
 * @name    Debug Levels (Hierarchical)
 * @{
 */
#define DBG_LEVEL_NONE          0       /**< No debug output (release)       */
#define DBG_LEVEL_ERROR         1       /**< Critical errors only            */
#define DBG_LEVEL_WARN          2       /**< Errors + warnings               */
#define DBG_LEVEL_INFO          3       /**< + state/mode changes            */
#define DBG_LEVEL_DEBUG         4       /**< + verbose details               */
/** @} */

/**
 * @brief   Active debug level.
 * @details Default is no debug output. Override via Makefile:
 *          -DAPP_DEBUG_LEVEL=X where X is the level (0-4).
 */
#ifndef APP_DEBUG_LEVEL
#define APP_DEBUG_LEVEL         DBG_LEVEL_NONE
#endif

/*===========================================================================*/
/* Debug Output Stream                                                       */
/*===========================================================================*/

/**
 * @brief   Debug output stream (USB CDC serial).
 */
#define DBG_STREAM              ((BaseSequentialStream*)&SDU1)

/**
 * @brief   Suppress unused variable warnings for debug-only variables.
 * @details Use this macro for variables that are only used in debug prints.
 */
#define DBG_UNUSED(x)           (void)(x)

/*===========================================================================*/
/* Debug Macros                                                              */
/*===========================================================================*/

/**
 * @brief   Error level debug macro.
 * @details Prints critical error messages. Always printed if level >= 1.
 *          Format: [ERROR] <message>
 *
 * @param[in] fmt   Printf-style format string.
 * @param[in] ...   Format arguments.
 */
#if (APP_DEBUG_LEVEL >= DBG_LEVEL_ERROR)
#define DBG_ERROR(fmt, ...) \
    chprintf(DBG_STREAM, "[ERROR] " fmt "\r\n", ##__VA_ARGS__)
#else
#define DBG_ERROR(fmt, ...) \
    do { } while (0)
#endif

/**
 * @brief   Warning level debug macro.
 * @details Prints warning messages. Printed if level >= 2.
 *          Format: [WARN] <message>
 *
 * @param[in] fmt   Printf-style format string.
 * @param[in] ...   Format arguments.
 */
#if (APP_DEBUG_LEVEL >= DBG_LEVEL_WARN)
#define DBG_WARN(fmt, ...) \
    chprintf(DBG_STREAM, "[WARN] " fmt "\r\n", ##__VA_ARGS__)
#else
#define DBG_WARN(fmt, ...) \
    do { } while (0)
#endif

/**
 * @brief   Info level debug macro.
 * @details Prints informational messages (state changes, mode changes).
 *          Printed if level >= 3.
 *          Format: [INFO] <message>
 *
 * @param[in] fmt   Printf-style format string.
 * @param[in] ...   Format arguments.
 */
#if (APP_DEBUG_LEVEL >= DBG_LEVEL_INFO)
#define DBG_INFO(fmt, ...) \
    chprintf(DBG_STREAM, "[INFO] " fmt "\r\n", ##__VA_ARGS__)
#else
#define DBG_INFO(fmt, ...) \
    do { } while (0)
#endif

/**
 * @brief   Debug level debug macro.
 * @details Prints verbose debug messages. Printed if level >= 4.
 *          Format: [DEBUG] <message>
 *
 * @param[in] fmt   Printf-style format string.
 * @param[in] ...   Format arguments.
 */
#if (APP_DEBUG_LEVEL >= DBG_LEVEL_DEBUG)
#define DBG_DEBUG(fmt, ...) \
    chprintf(DBG_STREAM, "[DEBUG] " fmt "\r\n", ##__VA_ARGS__)
#else
#define DBG_DEBUG(fmt, ...) \
    do { } while (0)
#endif

#endif /* APP_DEBUG_H */
