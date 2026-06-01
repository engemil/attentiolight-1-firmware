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
 * @file    al1_link_driver.h
 * @brief   STM32-side runtime for the AttentioLight-1 wireless-module UART link.
 *
 * @details Mirrors the ESP32 `al1_link` component. The wire format and the pure
 *          builder/parser live in al1_frame.h (a verbatim copy of the ESP32
 *          source); this module adds the ChibiOS SerialDriver runtime that
 *          carries it over USART1 (PB6=TX/AF0, PB7=RX/AF0, the WBM link).
 *
 *          Channels carry opaque payloads. The Attentio Protocol (its own
 *          CRC-8 framing) rides inside the AP_CTRL channel and is NOT parsed
 *          here; that bridge to MICB is a later phase.
 */

#ifndef AL1_LINK_DRIVER_H
#define AL1_LINK_DRIVER_H

#include "al1_frame.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Initialize the link module (parser + state). Call before start.
 */
void al1_link_init(void);

/**
 * @brief   Start USART1 (921600 8N1) and the RX thread.
 */
void al1_link_start(void);

/**
 * @brief   Frame and transmit a payload on a channel.
 * @param[in] channel   Channel byte (see al1_link_channel_t).
 * @param[in] payload   Payload bytes (may be NULL iff @p len == 0).
 * @param[in] len       Payload length (0..AL1_LINK_MAX_PAYLOAD).
 * @return  0 on success, nonzero on error.
 */
int al1_link_send(uint8_t channel, const uint8_t *payload, uint16_t len);

/**
 * @brief   Copy current link statistics into @p out.
 */
void al1_link_get_stats(al1_link_stats_t *out);

/**
 * @brief   Total raw bytes received on the link UART (pre-framing).
 * @details Diagnostic counter incremented for every byte read from USART1,
 *          regardless of whether it forms a valid frame. Lets a caller tell
 *          "nothing arriving" (stays 0) from "arriving but not framing"
 *          (climbs while al1_link_stats_t.rx_frames stays flat).
 */
uint32_t al1_link_get_rx_bytes(void);

#ifdef __cplusplus
}
#endif

#endif /* AL1_LINK_DRIVER_H */
