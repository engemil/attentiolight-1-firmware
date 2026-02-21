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

#ifndef APP_HEADER_H
#define APP_HEADER_H

#include <stdint.h>

/**
 * @brief Application header structure
 * 
 * This header must be placed at the start of the application (0x08004000)
 * to enable firmware validation by the bootloader.
 * 
 * The usb_vid and usb_pid fields allow the application to specify USB
 * identifiers that the bootloader will use in DFU mode.
 */
typedef struct __attribute__((packed)) {
    uint32_t magic;          /* Magic number: 0xDEADBEEF */
    uint32_t version;        /* Firmware version */
    uint32_t size;           /* Firmware size in bytes (excluding header) */
    uint32_t crc32;          /* CRC32 of firmware (excluding this header) */
    uint16_t usb_vid;        /* USB Vendor ID for bootloader DFU mode */
    uint16_t usb_pid;        /* USB Product ID for bootloader DFU mode */
    uint32_t reserved[3];    /* Reserved for future use */
} app_header_t;

#define APP_HEADER_MAGIC    0xDEADBEEF
#define APP_VERSION         0x00010000  /* Version 1.0.0 */

/* USB VID/PID - customize these or use defaults (STMicroelectronics DFU) */
#ifndef USB_VID
#define USB_VID             0x0483      /* STMicroelectronics */
#endif
#ifndef USB_PID
#define USB_PID             0xDF11      /* DFU mode */
#endif

#endif /* APP_HEADER_H */
