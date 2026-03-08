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
 *          make debug LEVEL=5    - Verbose + skip low-power mode
 *
 *          Debug levels are hierarchical:
 *            0 = NONE  - No output (release build)
 *            1 = ERROR - Critical errors only
 *            2 = WARN  - Errors + warnings
 *            3 = INFO  - Errors + warnings + state/mode changes
 *            4 = DEBUG - Everything (verbose details)
 *            5 = POWER - Verbose + disable Stop mode in off state
 *
 *          Level 5 (POWER) is useful during debugging as Stop mode halts
 *          all clocks including the debug interface, causing the debugger
 *          to lose connection. Use this level to keep the debugger
 *          connected while testing the off state behavior.
 */

#ifndef APP_DEBUG_H
#define APP_DEBUG_H

#include "ch.h"
#include "hal.h"
#include "chprintf.h"
#include "portab.h"
#include "usbcfg.h"
#include <stdarg.h>

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
#define DBG_LEVEL_POWER         5       /**< + skip low-power Stop mode      */
/** @} */

/**
 * @brief   Active debug level.
 * @details Default is no debug output. Override via Makefile:
 *          -DAPP_DEBUG_LEVEL=X where X is the level (0-4).
 */
#ifndef APP_DEBUG_LEVEL
#define APP_DEBUG_LEVEL         DBG_LEVEL_NONE
#endif

/**
 * @brief   Enable runtime stack watermark reporting.
 * @details Set to 1 to enable dbg_print_stack_usage(), which prints stack
 *          high-water marks for all threads over the debug serial port.
 *          Disabled by default. Only effective in debug builds (requires
 *          CH_DBG_FILL_THREADS and CH_CFG_USE_REGISTRY, both auto-enabled
 *          in debug builds via chconf.h).
 */
#define DBG_ENABLE_STACK_WATERMARK          0

/**
 * @brief   Debug output serial driver (USB CDC).
 */
#define DBG_SERIAL_DRIVER       PORTAB_SDU1

/**
 * @brief   Debug print buffer size.
 */
#define DBG_PRINT_BUF_SIZE      256

/**
 * @brief   Debug print timeout in milliseconds.
 */
#define DBG_PRINT_TIMEOUT_MS    200
#define DBG_PRINT_TIMEOUT       TIME_MS2I(DBG_PRINT_TIMEOUT_MS)

/*===========================================================================*/
/* Debug Output Stream                                                       */
/*===========================================================================*/

/**
 * @brief   Debug output stream (USB CDC serial).
 */
#define DBG_STREAM              ((BaseSequentialStream*)&DBG_SERIAL_DRIVER)

/**
 * @brief   Suppress unused variable warnings for debug-only variables.
 * @details Use this macro for variables that are only used in debug prints.
 */
#define DBG_UNUSED(x)           (void)(x)

/**
 * @brief   Debug print with timeout.
 * @details Formats message into buffer and writes to USB serial with timeout.
 *          Prevents blocking if USB host is not reading.
 *
 * @param[in] timeout   Timeout in system ticks (use TIME_MS2I(ms))
 * @param[in] fmt       Printf-style format string
 * @param[in] ...       Format arguments
 */
static inline void dbg_printf_timeout(sysinterval_t timeout, const char *fmt, ...) {
    char buf[DBG_PRINT_BUF_SIZE];
    va_list ap;

    va_start(ap, fmt);
    int len = chvsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    if (len > 0) {
        size_t written = chnWriteTimeout(&DBG_SERIAL_DRIVER, (uint8_t*)buf, (size_t)len, timeout);
        if (written < (size_t)len) {
            /* TODO: Handle timeout or buffer full (host not reading) */
        }
    }
}

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
    dbg_printf_timeout(DBG_PRINT_TIMEOUT, "[ERROR] " fmt "\r\n", ##__VA_ARGS__)
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
    dbg_printf_timeout(DBG_PRINT_TIMEOUT, "[WARN] " fmt "\r\n", ##__VA_ARGS__)
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
    dbg_printf_timeout(DBG_PRINT_TIMEOUT, "[INFO] " fmt "\r\n", ##__VA_ARGS__)
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
    dbg_printf_timeout(DBG_PRINT_TIMEOUT, "[DEBUG] " fmt "\r\n", ##__VA_ARGS__)
#else
#define DBG_DEBUG(fmt, ...) \
    do { } while (0)
#endif

/*===========================================================================*/
/* Low-Power Mode Control                                                    */
/*===========================================================================*/

/**
 * @brief   Macro to check if low-power Stop mode should be skipped.
 * @details When APP_DEBUG_LEVEL >= DBG_LEVEL_POWER (5), Stop mode is disabled
 *          to keep the debugger connected. The MCU will remain in normal run
 *          mode during the "off" state instead of entering Stop mode.
 *
 * @note    Use this in low-power code:
 *          @code
 *          #if !DBG_SKIP_LOW_POWER
 *              enter_stop_mode();
 *          #endif
 *          @endcode
 */
#define DBG_SKIP_LOW_POWER      (APP_DEBUG_LEVEL >= DBG_LEVEL_POWER)

/*===========================================================================*/
/* Stack Watermark Analysis                                                  */
/*===========================================================================*/

/**
 * @brief   Prints stack usage watermark for all threads.
 * @details Requires CH_DBG_FILL_THREADS and CH_CFG_USE_REGISTRY (both enabled
 *          automatically in debug builds). Iterates all registered threads and
 *          counts how many fill-pattern bytes (0x55) remain untouched from the
 *          bottom of each thread's working area.
 *
 *          Output format per thread:
 *            [STACK] <name>: <used>/<total> bytes (<percent>%)
 *
 * @note    Call from a thread context (not ISR). Typically called from main()
 *          after the system has been running for a while, or periodically.
 *
 * @note    This function itself uses ~40 bytes of stack. Call it from a thread
 *          with sufficient stack space (e.g. the main thread).
 *
 * Usage example:
 * @code
 *     // Print stack usage of all threads once
 *     dbg_print_stack_usage();
 *
 *     // Or periodically in a debug loop:
 *     while (true) {
 *         dbg_print_stack_usage();
 *         chThdSleepMilliseconds(5000);
 *     }
 * @endcode
 */
#if (DBG_ENABLE_STACK_WATERMARK == 1) && (CH_DBG_FILL_THREADS == TRUE) && \
    (CH_CFG_USE_REGISTRY == TRUE)
static inline void dbg_print_stack_usage(void) {
    thread_t *tp;

    dbg_printf_timeout(DBG_PRINT_TIMEOUT,
                       "\r\n[STACK] === Thread Stack Watermarks ===\r\n");

    tp = chRegFirstThread();
    while (tp != NULL) {
        /* Count untouched fill bytes (0x55) from the bottom of the WA */
        uint8_t *base = (uint8_t *)tp->wabase;
        uint8_t *end  = (uint8_t *)(tp + 1);  /* thread_t sits at top of WA */
        size_t total  = (size_t)(end - base);
        size_t free   = 0;

        while ((base + free) < end && base[free] == CH_DBG_STACK_FILL_VALUE) {
            free++;
        }

        size_t used = total - free;
        uint32_t pct = (total > 0) ? ((used * 100) / total) : 0;

        dbg_printf_timeout(DBG_PRINT_TIMEOUT,
                           "[STACK]  %-16s %4u / %4u bytes (%2lu%%)\r\n",
                           tp->name ? tp->name : "???",
                           (unsigned)used, (unsigned)total, (unsigned long)pct);

        tp = chRegNextThread(tp);
    }

    dbg_printf_timeout(DBG_PRINT_TIMEOUT,
                       "[STACK] ================================\r\n\r\n");
}
#else
static inline void dbg_print_stack_usage(void) {
    (void)0;
}
#endif

#endif /* APP_DEBUG_H */
