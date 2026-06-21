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
 * @file    al1_link_driver.c
 * @brief   STM32-side runtime for the wireless-module UART link (USART1/UARTD1).
 *
 * @details Uses the ChibiOS UART (DMA) driver rather than the IRQ-driven SERIAL
 *          driver: at 921600 baud the per-byte RX interrupt was starved by the
 *          higher-priority WS2812B timer IRQs, overrunning the 1-byte data
 *          register and corrupting inbound frames. DMA RX has no per-byte CPU
 *          interrupt, so it does not drop bytes under that contention.
 */

#include "al1_link_driver.h"
#include "app_log.h"
#include "rt_config.h"
#include "attentio_protocol.h"
#include "micb.h"

#include "ch.h"
#include "hal.h"

#include <string.h>

/*===========================================================================*/
/* Constants                                                                 */
/*===========================================================================*/

/** @brief WBM link UART (DMA driver). PB6=USART1_TX (AF0), PB7=USART1_RX (AF0). */
#define AL1_LINK_UART               (&UARTD1)
#define AL1_LINK_BAUD               921600

/** @brief RX read timeout, keeps the thread responsive without busy-spin. */
#define AL1_LINK_READ_TIMEOUT       TIME_MS2I(100)
/** @brief TX write timeout, don't block a sender forever if the wire stalls. */
#define AL1_LINK_WRITE_TIMEOUT      TIME_MS2I(200)
/** @brief Bulk RX chunk pulled from the UART per read. */
#define AL1_LINK_READ_BUF_SIZE      64

/*===========================================================================*/
/* Module state                                                              */
/*===========================================================================*/

/** @brief RX parser context (large, kept static, not on a thread stack). */
static al1_parser_t      al1_rx_parser;

/** @brief Running link counters. */
static al1_link_stats_t  al1_stats;

/** @brief Raw bytes received on USART1 (pre-framing; diagnostic only). */
static volatile uint32_t al1_rx_bytes;

/** @brief Per-channel TX sequence counters. */
static uint8_t           al1_tx_seq[256];

/** @brief Serializes frame build + UART write across senders. */
static mutex_t           al1_tx_mtx;

/** @brief True once al1_link_start() has brought up USART1 and the mutex.
 *         Lets al1_link_send() be safely called (and silently drop) before
 *         the link is up — e.g. early-boot LOG_* via log_printf_timeout(). */
static bool              al1_link_ready;

/** @brief TX scratch buffer (guarded by al1_tx_mtx). */
static uint8_t           al1_tx_buf[AL1_LINK_OVERHEAD + AL1_LINK_MAX_PAYLOAD];

/** @brief RX thread working area. */
static THD_WORKING_AREA(wa_al1_link_thread, AL1_LINK_THREAD_WA_SIZE);

/** @brief AP parser for the AP_CTRL channel (BLE interface stream). */
static ap_parser_t       al1_ap_parser;

/** @brief Scratch for AP error responses sent back over AP_CTRL. */
static uint8_t           al1_ap_err_buf[AP_MIN_PACKET_SIZE + 1];

/**
 * @brief   USART1 UART (DMA) config: 921600 8N1.
 * @details Callbacks unused (we use the blocking uartReceiveTimeout /
 *          uartSendTimeout API from the link thread). The receive `timeout`
 *          field is 0, so a short inter-character idle ends a partial DMA
 *          receive, letting uartReceiveTimeout return whatever has arrived.
 *          cr1 enables the 8-byte hardware RX FIFO for extra slack.
 */
static const UARTConfig al1_uartcfg = {
    .txend1_cb = NULL,
    .txend2_cb = NULL,
    .rxend_cb  = NULL,
    .rxchar_cb = NULL,
    .rxerr_cb  = NULL,
    .timeout_cb = NULL,
    .timeout   = 0,
    .speed     = AL1_LINK_BAUD,
    .cr1       = USART_CR1_FIFOEN,
    .cr2       = 0,
    .cr3       = 0,
};

/*===========================================================================*/
/* AP_CTRL <-> MICB bridge                                                    */
/*===========================================================================*/

/**
 * @brief   MICB send callback for the BLE interface.
 * @details MICB hands us a complete AP packet (response or event) for the
 *          active BLE controller; we ship it to the wireless module on the
 *          AP_CTRL channel, where the ESP32 routes it to the owning client.
 */
static void al1_link_ap_send(const uint8_t *data, size_t len) {
    if (data == NULL || len == 0) {
        return;
    }
    al1_link_send(AL1_CH_AP_CTRL, data, (uint16_t)len);
}

/**
 * @brief   Feed AP_CTRL payload bytes into the AP parser, dispatching complete
 *          packets to the MICB as the BLE interface (mirrors usb_adapter.c).
 */
static void al1_ap_ctrl_rx(const uint8_t *payload, uint16_t len) {
    for (uint16_t i = 0; i < len; i++) {
        ap_parse_result_t r = ap_parse_byte(&al1_ap_parser, payload[i]);

        switch (r) {
        case AP_PARSE_COMPLETE:
            micb_process_command(MICB_IF_BLE, &al1_ap_parser.pkt);
            ap_parser_reset(&al1_ap_parser);
            break;

        case AP_PARSE_ERR_CRC: {
            size_t n = ap_build_error(al1_ap_err_buf, sizeof(al1_ap_err_buf),
                                      AP_ERR_CRC_FAIL);
            if (n > 0) {
                al1_link_send(AL1_CH_AP_CTRL, al1_ap_err_buf, (uint16_t)n);
            }
            break; /* parser auto-resets on error */
        }

        case AP_PARSE_ERR_LEN:
        case AP_PARSE_ERR_OVERFLOW: {
            size_t n = ap_build_error(al1_ap_err_buf, sizeof(al1_ap_err_buf),
                                      AP_ERR_INVALID_PARAM);
            if (n > 0) {
                al1_link_send(AL1_CH_AP_CTRL, al1_ap_err_buf, (uint16_t)n);
            }
            break; /* parser auto-resets on error */
        }

        case AP_PARSE_NEED_MORE:
        default:
            break;
        }
    }
}

/*===========================================================================*/
/* RX frame dispatch                                                         */
/*===========================================================================*/

/**
 * @brief   Called by the parser for each complete, CRC-valid frame.
 */
static void al1_on_frame(uint8_t channel, uint8_t seq,
                         const uint8_t *payload, uint16_t len, void *user) {
    (void)user;

    switch (channel) {
    case AL1_CH_LOG:
        /* Surface the wireless module's log text on CDC0. Demoted from INFO to
         * DEBUG so it's off the UART-RX hot path at the default log level —
         * every LOG_* here can block up to LOG_PRINT_TIMEOUT on CDC0 when no
         * USB host is enumerated, which stalls AP_CTRL dispatch. Enable with
         * `attentio loglevel 4` when ESP32-side logs are needed. */
        LOG_DEBUG("WMOD LOG[seq=%u]: %.*s", seq, (int)len, (const char *)payload);
        break;

    case AL1_CH_AP_CTRL:
        /* Attentio Protocol from a BLE client -> MICB (via the ESP32 bridge). */
        al1_ap_ctrl_rx(payload, len);
        break;

    default:
        LOG_DEBUG("WMOD ch=0x%02x seq=%u len=%u", channel, seq, len);
        break;
    }
}

/*===========================================================================*/
/* RX thread                                                                 */
/*===========================================================================*/

static THD_FUNCTION(al1_link_thread, arg) {
    (void)arg;
    chRegSetThreadName("al1_link");

    LOG_SYS("AL1 link thread started (USART1 @ %d)", AL1_LINK_BAUD);

    uint8_t read_buf[AL1_LINK_READ_BUF_SIZE];

    while (true) {
        /*
         * DMA receive up to a bufferful. On MSG_TIMEOUT, uartReceiveTimeout
         * updates np to the bytes actually DMA'd in before the timeout/idle,
         * so partial frames are delivered promptly. MSG_OK means the buffer
         * filled. Either way np holds the byte count.
         */
        size_t np = sizeof(read_buf);
        msg_t m = uartReceiveTimeout(AL1_LINK_UART, &np, read_buf,
                                     AL1_LINK_READ_TIMEOUT);
        if ((m == MSG_OK || m == MSG_TIMEOUT) && np > 0) {
            al1_rx_bytes += (uint32_t)np;   /* raw bytes on the wire */
            al1_parser_feed(&al1_rx_parser, read_buf, np);
        }
    }
}

/*===========================================================================*/
/* Public API                                                                */
/*===========================================================================*/

void al1_link_init(void) {
    memset(&al1_stats, 0, sizeof(al1_stats));
    memset(al1_tx_seq, 0, sizeof(al1_tx_seq));
    al1_rx_bytes = 0;
    chMtxObjectInit(&al1_tx_mtx);
    al1_parser_init(&al1_rx_parser, al1_on_frame, NULL, &al1_stats);

    /* AP_CTRL carries the Attentio Protocol for BLE clients; parse it and
     * register the BLE interface so MICB can push responses/events back. */
    ap_parser_init(&al1_ap_parser);
    micb_register_interface(MICB_IF_BLE, al1_link_ap_send);

    LOG_DEBUG("AL1 link initialized");
}

void al1_link_start(void) {
    /*
     * PB6/PB7 are configured as alternate-function (AF0 = USART1) by the board
     * init (board.h). Start the UART (DMA) driver, then the RX thread.
     */
    uartStart(AL1_LINK_UART, &al1_uartcfg);

    chThdCreateStatic(wa_al1_link_thread, sizeof(wa_al1_link_thread),
                      AL1_LINK_THREAD_PRIORITY, al1_link_thread, NULL);

    al1_link_ready = true;
    LOG_DEBUG("AL1 link started");
}

int al1_link_send(uint8_t channel, const uint8_t *payload, uint16_t len) {
    /* Silently drop if the link hasn't been started yet (early-boot LOG_*
     * via log_printf_timeout, or any caller before al1_link_start()). */
    if (!al1_link_ready) {
        return -1;
    }

    if (len > AL1_LINK_MAX_PAYLOAD) {
        al1_stats.tx_drops++;
        return -1;
    }

    chMtxLock(&al1_tx_mtx);

    uint8_t seq = al1_tx_seq[channel]++;
    size_t frame_len = al1_frame_build(al1_tx_buf, sizeof(al1_tx_buf),
                                       channel, seq, payload, len);
    if (frame_len == 0) {
        chMtxUnlock(&al1_tx_mtx);
        al1_stats.tx_drops++;
        return -1;
    }

    size_t np = frame_len;
    msg_t m = uartSendTimeout(AL1_LINK_UART, &np, al1_tx_buf,
                              AL1_LINK_WRITE_TIMEOUT);
    int rc;
    if (m == MSG_OK && np == frame_len) {
        al1_stats.tx_frames++;
        rc = 0;
    } else {
        al1_stats.tx_drops++;
        rc = -1;
    }

    chMtxUnlock(&al1_tx_mtx);
    return rc;
}

uint32_t al1_link_get_rx_bytes(void) {
    return al1_rx_bytes;
}

void al1_link_get_stats(al1_link_stats_t *out) {
    if (out != NULL) {
        *out = al1_stats;
    }
}
