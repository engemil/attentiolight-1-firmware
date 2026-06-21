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
 * @details Provides runtime-configurable log output to CDC0 (USB serial)
 *          when a USB host has enumerated the device, and otherwise forwards
 *          each line over the wireless-module UART link (AL1_CH_LOG) so logs
 *          remain visible on `idf.py monitor` during BLE-only operation.
 *
 *          The log level is loaded from persistent storage at boot and can
 *          be changed at runtime via AP command.
 */

#include "app_log.h"
#include "persistent_data.h"
#include "al1_link_driver.h"
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
        /*
         * Route by USB enumeration state.
         *
         * ChibiOS only sets serusbcfg1.usbp->state == USB_ACTIVE once a USB
         * host has enumerated the device (the CONFIGURED event in the USB
         * event handler). With no host — e.g. powered from a dumb charger —
         * the SDU is never configured and chnWriteTimeout() to CDC0 blocks
         * for the full timeout (200 ms) per line because there is no IN
         * endpoint being ACKed. That stall, when it hits the al1_link UART
         * RX thread (which both re-logs ESP32 frames and dispatches AP_CTRL
         * to the MICB), overruns the UART RX FIFO and corrupts/loses BLE AP
         * frames — manifesting host-side as a 3 s command timeout.
         *
         * When USB is enumerated, write to CDC0 as before (reliable, and the
         * host reads the logs). When USB is not enumerated, forward the line
         * best-effort over AL1_CH_LOG to the ESP32 so it surfaces on
         * `idf.py monitor` without blocking the caller: al1_link_send queues a
         * frame and returns immediately.
         */
        if (serusbcfg1.usbp->state == USB_ACTIVE) {
            chnWriteTimeout(&LOG_SERIAL_DRIVER, (uint8_t*)buf, (size_t)len, timeout);
        }
        else {
            (void)al1_link_send(AL1_CH_LOG, (const uint8_t*)buf, (uint16_t)len);
        }
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
