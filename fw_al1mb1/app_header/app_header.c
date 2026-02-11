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
