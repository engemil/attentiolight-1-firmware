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
 * @file    app_log.h
 * @brief   Centralized logging system with runtime-configurable log levels.
 *
 * @details This header provides a unified logging system for the application.
 *          Unlike the old compile-time debug system (app_debug.h), all LOG_*
 *          code is always compiled in, regardless of build mode. Filtering
 *          happens at runtime via a global log level variable.
 *
 *          Build modes:
 *            make           - Release build (log code compiled in, default
 *                             log level loaded from persistent storage)
 *            make debug     - Debug build (-Og, -DAPP_DEBUG_BUILD=1, disables
 *                             Stop mode, enables stack watermark analysis)
 *
 *          Log levels (hierarchical):
 *            0 = NONE  - No output
 *            1 = ERROR - Critical errors only
 *            2 = WARN  - Errors + warnings
 *            3 = INFO  - Errors + warnings + info (state/mode changes)
 *            4 = DEBUG - Everything (verbose details)
 *
 *          The boot default log level is loaded from persistent storage
 *          (pd_data_t.log_level). It can be overridden at runtime via a
 *          dedicated AP command (0x60 range), without rebooting.
 *
 *          Output is sent to CDC0 (PORTAB_SDU1) with a write timeout to
 *          prevent blocking if the USB host is not reading.
 */

#ifndef APP_LOG_H
#define APP_LOG_H

#include "ch.h"
#include "hal.h"
#include "chprintf.h"
#include "portab.h"
#include "usbcfg.h"
#include <stdint.h>

/*===========================================================================*/
/* Log Level Definitions                                                     */
/*===========================================================================*/

/**
 * @name    Log Levels (Hierarchical)
 * @{
 */
#define LOG_LEVEL_NONE          0       /**< No log output                    */
#define LOG_LEVEL_ERROR         1       /**< Critical errors only             */
#define LOG_LEVEL_WARN          2       /**< Errors + warnings                */
#define LOG_LEVEL_INFO          3       /**< + state/mode changes             */
#define LOG_LEVEL_DEBUG         4       /**< + verbose details                */
/** @} */

/**
 * @brief   Default boot log level.
 * @details Used when persistent storage has no saved value (first boot) or
 *          when persistent_data_init() fails. The actual boot default comes
 *          from flash (pd_data_t.log_level) when available.
 */
#define LOG_DEFAULT_LEVEL       LOG_LEVEL_INFO

/*===========================================================================*/
/* Log Configuration                                                         */
/*===========================================================================*/

/**
 * @brief   Log output serial driver (USB CDC0).
 */
#define LOG_SERIAL_DRIVER       PORTAB_SDU1

/**
 * @brief   Log print buffer size.
 */
#define LOG_PRINT_BUF_SIZE      256

/**
 * @brief   Log print timeout in milliseconds.
 */
#define LOG_PRINT_TIMEOUT_MS    200
#define LOG_PRINT_TIMEOUT       TIME_MS2I(LOG_PRINT_TIMEOUT_MS)

/*===========================================================================*/
/* Runtime Log Level                                                         */
/*===========================================================================*/

/**
 * @brief   Global runtime log level.
 * @details Checked by LOG_* macros at runtime. Set at boot from persistent
 *          storage, overridable at runtime via AP command.
 *
 * @note    Declared volatile to ensure correct behavior across threads.
 *          Writes from any thread are atomic (single byte on ARM).
 */
extern volatile uint8_t g_log_level;

/*===========================================================================*/
/* Log Output Functions                                                      */
/*===========================================================================*/

/**
 * @brief   Log output stream (USB CDC serial).
 */
#define LOG_STREAM              ((BaseSequentialStream*)&LOG_SERIAL_DRIVER)

/**
 * @brief   Formatted log output with timeout.
 * @details Formats message into a stack buffer and writes to USB serial
 *          with a timeout. Prevents blocking if USB host is not reading.
 *
 * @param[in] timeout   Timeout in system ticks (use TIME_MS2I(ms))
 * @param[in] fmt       Printf-style format string
 * @param[in] ...       Format arguments
 */
void log_printf_timeout(sysinterval_t timeout, const char *fmt, ...);

/**
 * @brief   Initialize the log system.
 * @details Sets g_log_level from persistent storage. Call after
 *          persistent_data_init(). If persistent data is not available,
 *          uses LOG_DEFAULT_LEVEL.
 */
void log_init(void);

/**
 * @brief   Set the runtime log level.
 * @param[in] level  New log level (LOG_LEVEL_NONE to LOG_LEVEL_DEBUG).
 */
void log_set_level(uint8_t level);

/**
 * @brief   Get the current runtime log level.
 * @return  Current log level.
 */
uint8_t log_get_level(void);

/*===========================================================================*/
/* Log Macros                                                                */
/*===========================================================================*/

/**
 * @brief   Suppress unused variable warnings for log-only variables.
 * @details Use this macro for variables that are only referenced in LOG_*
 *          calls. Since LOG_* code is always compiled in, this is mainly for
 *          clarity and consistency.
 */
#define LOG_UNUSED(x)           (void)(x)

/**
 * @brief   Error level log macro.
 * @details Prints critical error messages. Output if g_log_level >= 1.
 *          Format: [ERROR] <message>
 *
 * @param[in] fmt   Printf-style format string.
 * @param[in] ...   Format arguments.
 */
#define LOG_ERROR(fmt, ...) \
    do { \
        if (g_log_level >= LOG_LEVEL_ERROR) { \
            log_printf_timeout(LOG_PRINT_TIMEOUT, "[ERROR] " fmt "\r\n", ##__VA_ARGS__); \
        } \
    } while (0)

/**
 * @brief   Warning level log macro.
 * @details Prints warning messages. Output if g_log_level >= 2.
 *          Format: [WARN] <message>
 *
 * @param[in] fmt   Printf-style format string.
 * @param[in] ...   Format arguments.
 */
#define LOG_WARN(fmt, ...) \
    do { \
        if (g_log_level >= LOG_LEVEL_WARN) { \
            log_printf_timeout(LOG_PRINT_TIMEOUT, "[WARN] " fmt "\r\n", ##__VA_ARGS__); \
        } \
    } while (0)

/**
 * @brief   Info level log macro.
 * @details Prints informational messages (state changes, mode changes).
 *          Output if g_log_level >= 3.
 *          Format: [INFO] <message>
 *
 * @param[in] fmt   Printf-style format string.
 * @param[in] ...   Format arguments.
 */
#define LOG_INFO(fmt, ...) \
    do { \
        if (g_log_level >= LOG_LEVEL_INFO) { \
            log_printf_timeout(LOG_PRINT_TIMEOUT, "[INFO] " fmt "\r\n", ##__VA_ARGS__); \
        } \
    } while (0)

/**
 * @brief   Debug level log macro.
 * @details Prints verbose debug messages. Output if g_log_level >= 4.
 *          Format: [DEBUG] <message>
 *
 * @param[in] fmt   Printf-style format string.
 * @param[in] ...   Format arguments.
 */
#define LOG_DEBUG(fmt, ...) \
    do { \
        if (g_log_level >= LOG_LEVEL_DEBUG) { \
            log_printf_timeout(LOG_PRINT_TIMEOUT, "[DEBUG] " fmt "\r\n", ##__VA_ARGS__); \
        } \
    } while (0)

/*===========================================================================*/
/* Stack Watermark Analysis                                                  */
/*===========================================================================*/

/**
 * @brief   Prints stack usage watermark for all threads.
 * @details Requires CH_DBG_FILL_THREADS and CH_CFG_USE_REGISTRY (both enabled
 *          automatically in debug builds via APP_DEBUG_BUILD). Iterates all
 *          registered threads and counts how many fill-pattern bytes (0x55)
 *          remain untouched from the bottom of each working area.
 *
 *          Output format per thread:
 *            [STACK] <name>: <used>/<total> bytes (<percent>%)
 *
 * @note    Call from a thread context (not ISR). Typically called from main()
 *          after the system has been running for a while, or periodically.
 *
 * @note    Only functional in debug builds (APP_DEBUG_BUILD=1) where
 *          CH_DBG_FILL_THREADS is enabled. In release builds this is a no-op.
 */
#if defined(APP_DEBUG_BUILD) && (APP_DEBUG_BUILD == 1) && \
    (CH_DBG_FILL_THREADS == TRUE) && (CH_CFG_USE_REGISTRY == TRUE)
static inline void log_print_stack_usage(void) {
    thread_t *tp;

    log_printf_timeout(LOG_PRINT_TIMEOUT,
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

        log_printf_timeout(LOG_PRINT_TIMEOUT,
                           "[STACK]  %-16s %4u / %4u bytes (%2lu%%)\r\n",
                           tp->name ? tp->name : "???",
                           (unsigned)used, (unsigned)total, (unsigned long)pct);

        tp = chRegNextThread(tp);
    }

    log_printf_timeout(LOG_PRINT_TIMEOUT,
                       "[STACK] ================================\r\n\r\n");
}
#else
static inline void log_print_stack_usage(void) {
    (void)0;
}
#endif

#endif /* APP_LOG_H */
