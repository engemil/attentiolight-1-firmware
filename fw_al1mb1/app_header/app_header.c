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

/*
 * Application Header for EngEmil STM32 Bootloader Integration
 * 
 * This file defines the application header structure that must be
 * placed at 0x08004000 (start of application flash region).
 * 
 * The bootloader validates this header before jumping to the application:
 * - Checks magic number (0xDEADBEEF)
 * - Validates firmware size
 * - Verifies CRC32 checksum
 * 
 * The size and crc32 fields are automatically signed by the
 * sign_app_header script during the build process.
 */

#include "app_header.h"

/* Application header placed at 0x08004000 via linker script */
__attribute__((section(".app_header")))
__attribute__((used))
const app_header_t app_header = {
    .magic = APP_HEADER_MAGIC,      /* 0xDEADBEEF */
    .version = APP_VERSION,          /* 0x00010000 (v1.0.0) */
    .size = 0,                       /* Signed by build script */
    .crc32 = 0,                      /* Signed by build script */
    .reserved = {0, 0, 0, 0}
};
