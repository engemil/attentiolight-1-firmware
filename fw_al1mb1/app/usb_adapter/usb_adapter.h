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
 * @file    usb_adapter.h
 * @brief   USB CDC1 adapter for Attentio Protocol (AP).
 *
 * @details The USB adapter runs a dedicated thread that reads bytes from
 *          CDC1 (PORTAB_SDU2), feeds them to the AP byte-by-byte parser,
 *          and dispatches complete packets to the MICB for processing.
 *
 *          Responses and unsolicited events are sent back on CDC1 via the
 *          registered send callback.
 */

#ifndef USB_ADAPTER_H
#define USB_ADAPTER_H

#include <stdint.h>

/*===========================================================================*/
/* Public API                                                                */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Initialize the USB adapter module.
 * @details Initializes the AP parser and prepares the adapter thread.
 *          Must be called after USB and MICB are initialized.
 */
void usb_adapter_init(void);

/**
 * @brief   Start the USB adapter thread.
 * @details Creates and starts the dedicated reader thread that processes
 *          incoming AP packets from CDC1.
 */
void usb_adapter_start(void);

#ifdef __cplusplus
}
#endif

#endif /* USB_ADAPTER_H */
