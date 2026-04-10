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
 * @file    app_log.c
 * @brief   Logging system implementation.
 *
 * @details Provides runtime-configurable log output to CDC0 (USB serial).
 *          The log level is loaded from persistent storage at boot and can
 *          be changed at runtime via AP command.
 */

#include "app_log.h"
#include "persistent_data.h"
#include <stdarg.h>

/*===========================================================================*/
/* Module local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Module exported variables.                                                */
/*===========================================================================*/

/**
 * @brief   Global runtime log level.
 * @details Initialized to LOG_DEFAULT_LEVEL. Updated by log_init() from
 *          persistent storage, and at runtime via log_set_level().
 */
volatile uint8_t g_log_level = LOG_DEFAULT_LEVEL;

/*===========================================================================*/
/* Module exported functions.                                                */
/*===========================================================================*/

void log_printf_timeout(sysinterval_t timeout, const char *fmt, ...) {
    char buf[LOG_PRINT_BUF_SIZE];
    va_list ap;

    va_start(ap, fmt);
    int len = chvsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    if (len > 0) {
        chnWriteTimeout(&LOG_SERIAL_DRIVER, (uint8_t*)buf, (size_t)len, timeout);
    }
}

void log_init(void) {
    const pd_data_t *pd = persistent_data_get();
    if (pd != NULL) {
        /* Load log level from persistent storage */
        if (pd->log_level <= LOG_LEVEL_DEBUG) {
            g_log_level = pd->log_level;
        } else {
            /* Invalid stored value, keep default */
            g_log_level = LOG_DEFAULT_LEVEL;
        }
    } else {
        /* Persistent data not available, use default */
        g_log_level = LOG_DEFAULT_LEVEL;
    }
}

void log_set_level(uint8_t level) {
    if (level <= LOG_LEVEL_DEBUG) {
        g_log_level = level;
    }
}

uint8_t log_get_level(void) {
    return g_log_level;
}
