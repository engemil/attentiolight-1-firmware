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
 * @file    debug_config.h
 * @brief   Compile-time debug diagnostics configuration.
 *
 * @details Centralizes flags that control optional debug-time diagnostic
 *          features.  These flags are independent of the runtime log level
 *          (g_log_level / LOG_* macros) — they control whether diagnostic
 *          code is compiled in at all, not how verbose the log output is.
 *
 *          All flags default to enabled (1) in debug builds
 *          (APP_DEBUG_BUILD=1, i.e. `make debug`) and disabled (0) in
 *          release builds.  Each flag can be individually overridden on the
 *          command line, e.g.:
 *
 *            make debug UDEFS="-DAPP_STACK_WATERMARK=0"
 *
 *          Companion files:
 *            - rt_config.h    — real-time parameters (priorities, stack sizes)
 *            - app_log.h      — runtime log level system and LOG_* macros
 *            - cfg/chconf.h   — ChibiOS kernel debug options
 *
 * @note    Include this header (directly or via app_log.h) wherever a
 *          debug flag is referenced.  The diagnostic functions themselves
 *          live in app_log.h.
 */

#ifndef DEBUG_CONFIG_H
#define DEBUG_CONFIG_H

/*===========================================================================*/
/* Build Mode Helper                                                         */
/*===========================================================================*/

/**
 * @brief   Resolves APP_DEBUG_BUILD into a simple 0/1 for use below.
 */
#ifdef APP_DEBUG_BUILD
#define APP_DEBUG_BUILD_PLUS_PLUS_ACTIVE          0 // Set to 1 if you want active in debug build

#else
#define APP_DEBUG_BUILD_PLUS_PLUS_ACTIVE          0
#endif

/*===========================================================================*/
/* Debug Diagnostic Flags                                                    */
/*===========================================================================*/
/*
 * Each flag defaults to APP_DEBUG_BUILD_PLUS_PLUS_ACTIVE (1 in debug, 0 in release).
 * Override on the command line:  make debug UDEFS="-DAPP_STACK_WATERMARK=0"
 *
 * These flags are independent of the runtime log level (g_log_level).
 * Setting LOG_LEVEL_NONE does NOT suppress diagnostic output — only
 * these flags control it.
 */

/* Periodic thread stack watermark reporting.  Output prefix: [STACK]
 * Prerequisites: CH_DBG_FILL_THREADS, CH_CFG_USE_REGISTRY (auto in debug). */
#ifndef APP_STACK_WATERMARK
#define APP_STACK_WATERMARK             APP_DEBUG_BUILD_PLUS_PLUS_ACTIVE
#endif

/* Periodic ChibiOS heap status reporting.  Output prefix: [HEAP]
 * Prerequisites: CH_CFG_USE_MEMCORE, CH_CFG_USE_HEAP. */
#ifndef APP_HEAP_ANALYSIS
#define APP_HEAP_ANALYSIS               APP_DEBUG_BUILD_PLUS_PLUS_ACTIVE
#endif

#endif /* DEBUG_CONFIG_H */
