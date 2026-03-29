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
 * @file    cmd_metadata.c
 * @brief   Shell command: metadata.
 *
 * @details Handles metadata query commands (read-only):
 *          - "metadata" or "metadata list" -> list all metadata fields
 *          - "metadata get <key>" -> read single metadata field
 *
 *          All metadata is read-only. No "set" subcommand exists.
 *
 *          Data sources:
 *          - device_model:       DEVICE_MODEL compile-time define
 *          - serial_number:      STM32C0xx UID register (alias for chip_uid)
 *          - firmware_version:   app_header.version
 *          - build_date:         __DATE__ " " __TIME__
 *          - chibios_kernel:     ChibiOS kernel version
 *          - chibios_port_info:  ChibiOS port info
 *          - compiler:           Compiler name/version
 *          - board:              Board name
 *          - hardware_revision:  HARDWARE_REVISION compile-time define
 *          - platform:           Platform name
 *          - chip_uid:           STM32C0xx UID register (0x1FFF7550)
 *          - architecture:       CPU architecture
 *          - core_variant:       Core variant name
 */

#include "cmd_metadata.h"
#include "shell_helpers.h"
#include "app_header.h"
#include "ch.h"
#include "hal.h"
#include "chprintf.h"
#include <string.h>

/*===========================================================================*/
/* Local definitions                                                         */
/*===========================================================================*/

/**
 * @brief   STM32C0xx unique device ID register base address.
 * @note    96-bit UID stored as 3 x 32-bit words at 0x1FFF7550.
 */
#define STM32_UID_BASE      0x1FFF7550U

/**
 * @brief   Build date/time string from compiler macros.
 */
#define BUILD_DATE_STRING   __DATE__ " " __TIME__

/**
 * @brief   Number of metadata fields.
 */
#define METADATA_FIELD_COUNT    13U

/**
 * @brief   Metadata field names (must match the order in print_field).
 */
static const char * const metadata_field_names[METADATA_FIELD_COUNT] = {
    "device_model",
    "serial_number",
    "firmware_version",
    "build_date",
    "chibios_kernel",
    "chibios_port_info",
    "compiler",
    "board",
    "hardware_revision",
    "platform",
    "chip_uid",
    "architecture",
    "core_variant"
};

/*===========================================================================*/
/* Local helper functions                                                    */
/*===========================================================================*/

/**
 * @brief   Format firmware version from app_header into a buffer.
 *
 * @param[out] buf   Output buffer (must be at least 12 bytes).
 * @param[in]  size  Buffer size.
 */
static void format_firmware_version(char *buf, size_t size) {
    uint32_t ver = app_header.version;
    uint8_t major = (uint8_t)((ver >> 16) & 0xFFU);
    uint8_t minor = (uint8_t)((ver >> 8) & 0xFFU);
    uint8_t patch = (uint8_t)(ver & 0xFFU);

    chsnprintf(buf, size, "%u.%u.%u", major, minor, patch);
}

/**
 * @brief   Format chip UID as a hex string.
 *
 * @param[out] buf   Output buffer (must be at least 25 bytes for 24 hex chars + null).
 * @param[in]  size  Buffer size.
 */
static void format_chip_uid(char *buf, size_t size) {
    const uint32_t *uid = (const uint32_t *)STM32_UID_BASE;

    chsnprintf(buf, size, "%08X%08X%08X", uid[0], uid[1], uid[2]);
}

/**
 * @brief   Print a single metadata field by name.
 *
 * @param[in]  chp   Shell output stream.
 * @param[in]  name  Field name to print.
 *
 * @return  true if field was found and printed, false otherwise.
 */
static bool print_field(BaseSequentialStream *chp, const char *name) {
    char buf[32];

    if (strcmp(name, "serial_number") == 0) {
        format_chip_uid(buf, sizeof(buf));
        chprintf(chp, "serial_number=%s\r\n", buf);
        return true;
    }

    if (strcmp(name, "firmware_version") == 0) {
        format_firmware_version(buf, sizeof(buf));
        chprintf(chp, "firmware_version=%s\r\n", buf);
        return true;
    }

    if (strcmp(name, "device_model") == 0) {
        chprintf(chp, "device_model=%s\r\n", DEVICE_MODEL);
        return true;
    }

    if (strcmp(name, "hardware_revision") == 0) {
        chprintf(chp, "hardware_revision=%s\r\n", HARDWARE_REVISION);
        return true;
    }

    if (strcmp(name, "build_date") == 0) {
        chprintf(chp, "build_date=%s\r\n", BUILD_DATE_STRING);
        return true;
    }

    if (strcmp(name, "chip_uid") == 0) {
        format_chip_uid(buf, sizeof(buf));
        chprintf(chp, "chip_uid=%s\r\n", buf);
        return true;
    }

    if (strcmp(name, "chibios_kernel") == 0) {
        chprintf(chp, "chibios_kernel=%s\r\n", CH_KERNEL_VERSION);
        return true;
    }

    if (strcmp(name, "compiler") == 0) {
#ifdef PORT_COMPILER_NAME
        chprintf(chp, "compiler=%s\r\n", PORT_COMPILER_NAME);
#else
        chprintf(chp, "compiler=N/A\r\n");
#endif
        return true;
    }

    if (strcmp(name, "architecture") == 0) {
        chprintf(chp, "architecture=%s\r\n", PORT_ARCHITECTURE_NAME);
        return true;
    }

    if (strcmp(name, "core_variant") == 0) {
#ifdef PORT_CORE_VARIANT_NAME
        chprintf(chp, "core_variant=%s\r\n", PORT_CORE_VARIANT_NAME);
#else
        chprintf(chp, "core_variant=N/A\r\n");
#endif
        return true;
    }

    if (strcmp(name, "chibios_port_info") == 0) {
#ifdef PORT_INFO
        chprintf(chp, "chibios_port_info=%s\r\n", PORT_INFO);
#else
        chprintf(chp, "chibios_port_info=N/A\r\n");
#endif
        return true;
    }

    if (strcmp(name, "platform") == 0) {
#ifdef PLATFORM_NAME
        chprintf(chp, "platform=%s\r\n", PLATFORM_NAME);
#else
        chprintf(chp, "platform=N/A\r\n");
#endif
        return true;
    }

    if (strcmp(name, "board") == 0) {
#ifdef BOARD_NAME
        chprintf(chp, "board=%s\r\n", BOARD_NAME);
#else
        chprintf(chp, "board=N/A\r\n");
#endif
        return true;
    }

    return false;
}

/**
 * @brief   Handle "metadata list" or "metadata" (no args).
 */
static void cmd_metadata_list(BaseSequentialStream *chp) {
    for (size_t i = 0; i < METADATA_FIELD_COUNT; i++) {
        print_field(chp, metadata_field_names[i]);
    }
    shell_ok(chp);
}

/**
 * @brief   Handle "metadata get <key>".
 */
static void cmd_metadata_get(BaseSequentialStream *chp, const char *key) {
    if (!print_field(chp, key)) {
        shell_error(chp, "unknown key");
        return;
    }
    shell_ok(chp);
}

/*===========================================================================*/
/* Command handler                                                           */
/*===========================================================================*/

void cmd_metadata(BaseSequentialStream *chp, int argc, char *argv[]) {

    /* No arguments or "list" subcommand -> list all metadata */
    if (argc == 0 || (argc == 1 && strcmp(argv[0], "list") == 0)) {
        cmd_metadata_list(chp);
        return;
    }

    if (argc < 2) {
        shell_error(chp, "missing arguments");
        return;
    }

    const char *subcmd = argv[0];
    const char *key = argv[1];

    if (strcmp(subcmd, "get") == 0) {
        cmd_metadata_get(chp, key);
    }
    else {
        shell_error(chp, "unknown subcommand");
    }
}
