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
 * @file    persistent_data.h
 * @brief   Persistent settings storage module (user-configurable values).
 * @note    Stores read-write settings in EFL page 1. Separate from device
 *          metadata (EFL page 0) so that factory resets only clear settings
 *          without affecting production-programmed metadata.
 *
 * @details Settings are values the user can change (e.g. device_name).
 *          Read-only device identity data is handled by the device_metadata
 *          module instead.
 *
 * @addtogroup PERSISTENT_DATA
 * @{
 */

#ifndef PERSISTENT_DATA_H
#define PERSISTENT_DATA_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/*===========================================================================*/
/* Module constants.                                                         */
/*===========================================================================*/

/**
 * @name    Field size definitions
 * @{
 */
#define PD_DEVICE_NAME_SIZE     32U     /**< Device name field size (bytes)   */
/** @} */

/**
 * @name    Storage format constants
 * @{
 */
#define PD_MAGIC                0x50444154U /**< Magic number "PDAT"          */
#define PD_VERSION              0x0003U     /**< Data format version          */
/** @} */

/*===========================================================================*/
/* Module data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Result codes for persistent data operations.
 */
typedef enum {
    PD_OK                   = 0,    /**< Operation successful                 */
    PD_ERROR_NOT_INIT       = 1,    /**< Module not initialized               */
    PD_ERROR_BUFFER_SIZE    = 2,    /**< Buffer too small / data too large    */
    PD_ERROR_EFL_START      = 3,    /**< Failed to start EFL driver           */
    PD_ERROR_EFL_ERASE      = 4,    /**< Failed to erase EFL sector           */
    PD_ERROR_EFL_PROGRAM    = 5,    /**< Failed to program EFL                */
    PD_ERROR_EFL_READ       = 6,    /**< Failed to read EFL                   */
    PD_ERROR_EFL_STOP       = 7,    /**< Failed to stop EFL driver            */
    PD_ERROR_CRC            = 8,    /**< CRC validation failed                */
    PD_ERROR_STORAGE        = 9,    /**< Storage validation failed            */
} pd_result_t;

/**
 * @brief   Persistent settings data structure.
 * @note    Contains only user-configurable settings. All fields are
 *          read-write. Add future settings here.
 */
typedef struct {
    char device_name[PD_DEVICE_NAME_SIZE];      /**< User-assigned name       */
    uint8_t log_level;                          /**< Persistent log level     */
    uint8_t _reserved[3];                       /**< Alignment padding        */
} pd_data_t;

/*===========================================================================*/
/* Module public functions.                                                  */
/*===========================================================================*/

/**
 * @name    Initialization
 * @{
 */

/**
 * @brief   Initialize the persistent data module.
 * @note    Loads settings from EFL page 1. If the data is invalid or
 *          uninitialized, factory defaults are loaded and saved.
 *
 * @return  Result code indicating success or failure reason.
 */
pd_result_t persistent_data_init(void);

/** @} */

/**
 * @name    Data access
 * @{
 */

/**
 * @brief   Get pointer to the settings data structure.
 * @note    Returns a const pointer to prevent accidental modification.
 *          Use the setter functions to modify settings.
 * @pre     persistent_data_init() must have been called.
 *
 * @return  Pointer to the settings data, or NULL if not initialized.
 */
const pd_data_t *persistent_data_get(void);

/**
 * @brief   Set the device name.
 * @note    Updates the RAM cache. Call persistent_data_save() to persist.
 * @pre     persistent_data_init() must have been called.
 *
 * @param[in] name  New device name (null-terminated string).
 *
 * @return  Result code.
 * @retval  PD_OK               Write to RAM cache successful.
 * @retval  PD_ERROR_NOT_INIT   Module not initialized.
 * @retval  PD_ERROR_BUFFER_SIZE  Name too long (max PD_DEVICE_NAME_SIZE - 1).
 */
pd_result_t persistent_data_set_device_name(const char *name);

/**
 * @brief   Set the persistent log level.
 * @note    Updates the RAM cache. Call persistent_data_save() to persist.
 * @pre     persistent_data_init() must have been called.
 *
 * @param[in] level  New log level (0-4).
 *
 * @return  Result code.
 * @retval  PD_OK               Write to RAM cache successful.
 * @retval  PD_ERROR_NOT_INIT   Module not initialized.
 * @retval  PD_ERROR_BUFFER_SIZE  Level out of range.
 */
pd_result_t persistent_data_set_log_level(uint8_t level);

/** @} */

/**
 * @name    Persistence operations
 * @{
 */

/**
 * @brief   Save current settings to EFL storage.
 * @pre     persistent_data_init() must have been called.
 *
 * @return  Result code indicating success or failure reason.
 */
pd_result_t persistent_data_save(void);

/**
 * @brief   Reload settings from EFL storage.
 * @note    Discards any unsaved changes in RAM cache.
 * @pre     persistent_data_init() must have been called.
 *
 * @return  Result code indicating success or failure reason.
 */
pd_result_t persistent_data_load(void);

/**
 * @brief   Reset all settings to factory defaults.
 * @note    Loads defaults into RAM cache and saves to flash.
 *          Does NOT affect device metadata (separate EFL page).
 * @pre     persistent_data_init() must have been called.
 *
 * @return  Result code indicating success or failure reason.
 */
pd_result_t persistent_data_factory_reset(void);

/** @} */

/**
 * @name    Utility functions
 * @{
 */

/**
 * @brief   Get a human-readable string for a result code.
 *
 * @param[in] result  Result code.
 *
 * @return  String describing the result.
 */
const char *persistent_data_result_str(pd_result_t result);

/**
 * @brief   Check if the module is initialized.
 *
 * @return  true if initialized, false otherwise.
 */
bool persistent_data_is_initialized(void);

/** @} */

#endif /* PERSISTENT_DATA_H */

/** @} */
