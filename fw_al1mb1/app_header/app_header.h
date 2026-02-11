#ifndef APP_HEADER_H
#define APP_HEADER_H

#include <stdint.h>

/**
 * @brief Application header structure
 * 
 * This header must be placed at the start of the application (0x08004000)
 * to enable firmware validation by the bootloader.
 */
typedef struct __attribute__((packed)) {
    uint32_t magic;          /* Magic number: 0xDEADBEEF */
    uint32_t version;        /* Firmware version */
    uint32_t size;           /* Firmware size in bytes (excluding header) */
    uint32_t crc32;          /* CRC32 of firmware (excluding this header) */
    uint32_t reserved[4];    /* Reserved for future use */
} app_header_t;

#define APP_HEADER_MAGIC    0xDEADBEEF
#define APP_VERSION         0x00010000  /* Version 1.0.0 */

#endif /* APP_HEADER_H */
