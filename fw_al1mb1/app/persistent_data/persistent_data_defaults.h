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
 * @file    persistent_data_defaults.h
 * @brief   Factory default values for persistent settings.
 * @note    Modify these values to change the defaults that are loaded
 *          on first boot or after a factory reset.
 *          Device metadata defaults are defined in device_metadata.h
 *          instead.
 *
 * @addtogroup PERSISTENT_DATA
 * @{
 */

#ifndef PERSISTENT_DATA_DEFAULTS_H
#define PERSISTENT_DATA_DEFAULTS_H

/*===========================================================================*/
/* Factory default values.                                                   */
/*===========================================================================*/

/**
 * @brief   Default device name.
 * @note    Maximum length is PD_DEVICE_NAME_SIZE - 1 (31 characters).
 */
#define PD_DEFAULT_DEVICE_NAME      "AttentioLight-1"

/**
 * @brief   Default log level.
 * @note    3 = INFO (errors + warnings + state/mode changes).
 *          See LOG_LEVEL_* definitions in app_log.h.
 */
#define PD_DEFAULT_LOG_LEVEL        3

#endif /* PERSISTENT_DATA_DEFAULTS_H */

/** @} */
