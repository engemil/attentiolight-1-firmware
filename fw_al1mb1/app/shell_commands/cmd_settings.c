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
 * @details Handles user-configurable settings (all read-write):
 *          - "settings" or "settings list" -> list all settings
 *          - "settings get <key>" -> read single setting
 *          - "settings set <key> <value>" -> write single setting
 *
 *          Read-only device metadata (serial number, firmware version, etc.)
 *          is handled by the "metadata" command instead.
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
 * @brief   Handle "settings list" or "settings" (no args).
 * @note    Outputs all settings in key=value format.
 */
static void cmd_settings_list(BaseSequentialStream *chp) {
    const pd_data_t *pd = persistent_data_get();

    if (pd == NULL) {
        shell_error(chp, "not initialized");
        return;
    }

    chprintf(chp, "device_name=%s\r\n", pd->device_name);
    /* Add future settings output here */

    shell_ok(chp);
}

/**
 * @brief   Handle "settings get <key>".
 */
static void cmd_settings_get(BaseSequentialStream *chp, const char *key) {
    const pd_data_t *pd = persistent_data_get();

    if (pd == NULL) {
        shell_error(chp, "not initialized");
        return;
    }

    if (strcmp(key, "device_name") == 0) {
        shell_value(chp, pd->device_name);
        shell_ok(chp);
    }
    /* Add future settings get handlers here */
    else {
        shell_error(chp, "unknown key");
    }
}

/**
 * @brief   Handle "settings set <key> <value>".
 */
static void cmd_settings_set(BaseSequentialStream *chp, const char *key,
                             const char *value) {

    if (strcmp(key, "device_name") == 0) {
        pd_result_t result = persistent_data_set_device_name(value);
        if (result != PD_OK) {
            shell_error(chp, persistent_data_result_str(result));
            return;
        }
    }
    /* Add future settings set handlers here */
    else {
        shell_error(chp, "unknown key");
        return;
    }

    /* Persist to flash */
    pd_result_t result = persistent_data_save();
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

    /* No arguments or "list" subcommand -> list all settings */
    if (argc == 0 || (argc == 1 && strcmp(argv[0], "list") == 0)) {
        cmd_settings_list(chp);
        return;
    }

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
