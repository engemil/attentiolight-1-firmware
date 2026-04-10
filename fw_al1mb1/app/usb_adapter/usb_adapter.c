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
 * @file    usb_adapter.c
 * @brief   USB CDC1 adapter for Attentio Protocol (AP).
 *
 * @details Implements a dedicated thread that:
 *          1. Reads bytes from CDC1 (PORTAB_SDU2) with a timeout.
 *          2. Feeds each byte into the AP byte-by-byte parser.
 *          3. On complete valid packet: dispatches to MICB.
 *          4. On parse errors: sends an AP error response back on CDC1.
 *
 *          The send callback (usb_adapter_send) is registered with MICB so
 *          that responses and unsolicited events can be pushed to the host.
 */

#include "usb_adapter.h"
#include "attentio_protocol.h"
#include "micb.h"
#include "app_log.h"
#include "rt_config.h"
#include "usbcfg.h"

#include "ch.h"
#include "hal.h"

/*===========================================================================*/
/* Constants                                                                 */
/*===========================================================================*/

/**
 * @brief   Read timeout for CDC1 in system ticks.
 * @details The thread blocks on chnReadTimeout() for up to this duration.
 *          100ms keeps the thread responsive while avoiding busy-spinning.
 */
#define USB_ADAPTER_READ_TIMEOUT    TIME_MS2I(100)

/**
 * @brief   Write timeout for CDC1 in system ticks.
 * @details Timeout for sending response/event bytes back to host.
 *          500ms is generous; if the host isn't reading, we don't want
 *          to block the MICB response path indefinitely.
 */
#define USB_ADAPTER_WRITE_TIMEOUT   TIME_MS2I(500)

/**
 * @brief   Read buffer size for bulk reads from CDC1.
 * @details We read multiple bytes at once for efficiency rather than
 *          calling chnReadTimeout() for each byte individually.
 *          64 bytes matches the USB full-speed bulk endpoint size.
 */
#define USB_ADAPTER_READ_BUF_SIZE   64

/*===========================================================================*/
/* Module local variables                                                    */
/*===========================================================================*/

/** @brief AP parser context for incoming bytes from CDC1. */
static ap_parser_t usb_parser;

/** @brief Response buffer for sending parse error responses. */
static uint8_t err_resp_buf[AP_MIN_PACKET_SIZE + 1];

/** @brief Thread working area. */
static THD_WORKING_AREA(wa_usb_adapter_thread, RT_USB_ADAPTER_THREAD_WA_SIZE);

/*===========================================================================*/
/* Send callback                                                             */
/*===========================================================================*/

/**
 * @brief   Send callback registered with MICB for the USB interface.
 * @details Writes raw AP packet bytes to CDC1 (PORTAB_SDU2).
 *
 * @param[in] data      Raw AP packet bytes.
 * @param[in] len       Number of bytes to send.
 */
static void usb_adapter_send(const uint8_t *data, size_t len) {
    if (data == NULL || len == 0) {
        return;
    }
    chnWriteTimeout(&PORTAB_SDU2, data, len, USB_ADAPTER_WRITE_TIMEOUT);
}

/*===========================================================================*/
/* Thread function                                                           */
/*===========================================================================*/

/**
 * @brief   USB adapter thread function.
 * @details Continuously reads bytes from CDC1, parses AP packets, and
 *          dispatches complete packets to the MICB.
 *
 * @param[in] arg   Unused.
 * @return          Should never return.
 */
static THD_FUNCTION(usb_adapter_thread, arg) {
    (void)arg;

    chRegSetThreadName("usb_ap");

    LOG_INFO("USB-AP adapter thread started");

    uint8_t read_buf[USB_ADAPTER_READ_BUF_SIZE];

    while (true) {
        /* Read a chunk of bytes from CDC1 with timeout. */
        size_t n = chnReadTimeout(&PORTAB_SDU2, read_buf, sizeof(read_buf),
                                  USB_ADAPTER_READ_TIMEOUT);

        /* Process each received byte through the AP parser. */
        for (size_t i = 0; i < n; i++) {
            ap_parse_result_t result = ap_parse_byte(&usb_parser, read_buf[i]);

            switch (result) {
            case AP_PARSE_COMPLETE:
                /* Valid packet — dispatch to MICB. */
                micb_process_command(MICB_IF_USB, &usb_parser.pkt);
                ap_parser_reset(&usb_parser);
                break;

            case AP_PARSE_ERR_CRC:
                LOG_WARN("USB-AP CRC error");
                {
                    size_t resp_len = ap_build_error(err_resp_buf,
                                                     sizeof(err_resp_buf),
                                                     AP_ERR_CRC_FAIL);
                    if (resp_len > 0) {
                        usb_adapter_send(err_resp_buf, resp_len);
                    }
                }
                /* Parser auto-resets on error. */
                break;

            case AP_PARSE_ERR_LEN:
            case AP_PARSE_ERR_OVERFLOW:
                LOG_WARN("USB-AP frame error (len)");
                {
                    size_t resp_len = ap_build_error(err_resp_buf,
                                                     sizeof(err_resp_buf),
                                                     AP_ERR_INVALID_PARAM);
                    if (resp_len > 0) {
                        usb_adapter_send(err_resp_buf, resp_len);
                    }
                }
                /* Parser auto-resets on error. */
                break;

            case AP_PARSE_NEED_MORE:
                /* Normal — need more bytes to complete packet. */
                break;
            }
        }
    }
}

/*===========================================================================*/
/* Public API                                                                */
/*===========================================================================*/

void usb_adapter_init(void) {
    ap_parser_init(&usb_parser);

    /* Register send callback with MICB. */
    micb_register_interface(MICB_IF_USB, usb_adapter_send);

    LOG_DEBUG("USB-AP adapter initialized");
}

void usb_adapter_start(void) {
    chThdCreateStatic(wa_usb_adapter_thread,
                      sizeof(wa_usb_adapter_thread),
                      RT_USB_ADAPTER_THREAD_PRIORITY,
                      usb_adapter_thread,
                      NULL);

    LOG_DEBUG("USB-AP adapter thread created");
}
