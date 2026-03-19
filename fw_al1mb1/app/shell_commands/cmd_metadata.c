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
 *          - serial_number:      EFL page 0 (device_metadata module)
 *          - firmware_version:   app_header.version
 *          - device_model:       DEVICE_MODEL compile-time define
 *          - hardware_revision:  HARDWARE_REVISION compile-time define
 *          - build_date:         __DATE__ " " __TIME__
 *          - chip_uid:           STM32C0xx UID register (0x1FFF7550)
 */

#include "cmd_metadata.h"
#include "shell_helpers.h"
#include "device_metadata.h"
#include "app_header.h"
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
#define METADATA_FIELD_COUNT    6U

/**
 * @brief   Metadata field names (must match the order in print_field).
 */
static const char * const metadata_field_names[METADATA_FIELD_COUNT] = {
    "serial_number",
    "firmware_version",
    "device_model",
    "hardware_revision",
    "build_date",
    "chip_uid",
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
        const md_data_t *md = device_metadata_get();
        if (md != NULL) {
            chprintf(chp, "serial_number=%s\r\n", md->serial_number);
        } else {
            chprintf(chp, "serial_number=ERROR\r\n");
        }
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
