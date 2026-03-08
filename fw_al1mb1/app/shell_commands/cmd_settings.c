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
 * @file    cmd_settings.c
 * @brief   Shell command: settings.
 *
 * @details Handles "settings get <key>" and "settings set <key> <value>"
 *          commands by interfacing with the persistent data module.
 *
 * @example
 *          -> settings get serial_number\r\n
 *          <- 000000000000\r\n
 *          <- OK\r\n
 *
 *          -> settings set device_name "My Light"\r\n
 *          <- OK\r\n
 *
 *          -> settings set serial_number XYZ\r\n
 *          <- ERROR key is read-only\r\n
 *
 *          -> settings get nonexistent\r\n
 *          <- ERROR unknown key\r\n
 */

#include "cmd_settings.h"
#include "shell_helpers.h"
#include "persistent_data.h"
#include "chprintf.h"
#include <string.h>

/*===========================================================================*/
/* Local helper functions                                                    */
/*===========================================================================*/

/**
 * @brief   Handle "settings get <key>".
 */
static void cmd_settings_get(BaseSequentialStream *chp, const char *key) {
    pd_field_info_t info;

    /* Look up field by name */
    if (!persistent_data_find_field_by_name(key, &info)) {
        shell_error(chp, "unknown key");
        return;
    }

    /* Check access level (INTERNAL fields cannot be read externally) */
    if (info.access == PD_ACCESS_INTERNAL) {
        shell_error(chp, "unknown key");
        return;
    }

    /* Read the field value */
    char buf[PD_DEVICE_NAME_SIZE];  /* Use largest field size as buffer */
    size_t size = sizeof(buf);
    pd_result_t result = persistent_data_read_field(info.id, buf, &size);

    if (result != PD_OK) {
        shell_error(chp, persistent_data_result_str(result));
        return;
    }

    /* Ensure null-termination for string fields */
    buf[sizeof(buf) - 1] = '\0';

    shell_value(chp, buf);
    shell_ok(chp);
}

/**
 * @brief   Handle "settings set <key> <value>".
 */
static void cmd_settings_set(BaseSequentialStream *chp, const char *key,
                             const char *value) {
    pd_field_info_t info;

    /* Look up field by name */
    if (!persistent_data_find_field_by_name(key, &info)) {
        shell_error(chp, "unknown key");
        return;
    }

    /* Check access level */
    if (info.access != PD_ACCESS_RW) {
        shell_error(chp, "key is read-only");
        return;
    }

    /* Write the field value (include null terminator for string fields) */
    size_t len = strlen(value) + 1;
    pd_result_t result = persistent_data_write_field(info.id, value, len);

    if (result != PD_OK) {
        shell_error(chp, persistent_data_result_str(result));
        return;
    }

    /* Persist to flash */
    result = persistent_data_save();
    if (result != PD_OK) {
        shell_error(chp, persistent_data_result_str(result));
        return;
    }

    shell_ok(chp);
}

/*===========================================================================*/
/* Command handler                                                           */
/*===========================================================================*/

void cmd_settings(BaseSequentialStream *chp, int argc, char *argv[]) {

    if (argc < 2) {
        shell_error(chp, "missing arguments");
        return;
    }

    const char *subcmd = argv[0];
    const char *key = argv[1];

    if (strcmp(subcmd, "get") == 0) {
        cmd_settings_get(chp, key);
    }
    else if (strcmp(subcmd, "set") == 0) {
        if (argc < 3) {
            shell_error(chp, "missing value");
            return;
        }
        cmd_settings_set(chp, key, argv[2]);
    }
    else {
        shell_error(chp, "unknown subcommand");
    }
}
