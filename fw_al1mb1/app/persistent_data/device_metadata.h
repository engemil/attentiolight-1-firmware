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
 * @file    device_metadata.h
 * @brief   Read-only device metadata storage module.
 * @note    Stores production-programmed device identity data (e.g. serial
 *          number) in EFL page 0. Separate from user settings to ensure
 *          metadata survives factory resets.
 *
 * @details Metadata is read-only over the shell interface. Values are
 *          programmed during production and never modified by the user.
 *          Runtime metadata (firmware version, chip UID, build date) is
 *          gathered from native sources, not stored here.
 *
 * @addtogroup DEVICE_METADATA
 * @{
 */

#ifndef DEVICE_METADATA_H
#define DEVICE_METADATA_H

#include <stdint.h>
#include <stdbool.h>

/*===========================================================================*/
/* Module constants.                                                         */
/*===========================================================================*/

/**
 * @name    Field size definitions
 * @{
 */
#define MD_SERIAL_NUMBER_SIZE   16U     /**< Serial number field size (bytes) */
/** @} */

/**
 * @name    Storage format constants
 * @{
 */
#define MD_MAGIC                0x4D444154U /**< Magic number "MDAT"          */
#define MD_VERSION              0x0001U     /**< Data format version          */
/** @} */

/**
 * @name    Default values
 * @{
 */
#define MD_DEFAULT_SERIAL_NUMBER    "000000000000"
/** @} */

/*===========================================================================*/
/* Module data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Result codes for metadata operations.
 */
typedef enum {
    MD_OK                   = 0,    /**< Operation successful                 */
    MD_ERROR_NOT_INIT       = 1,    /**< Module not initialized               */
    MD_ERROR_EFL_START      = 2,    /**< Failed to start EFL driver           */
    MD_ERROR_EFL_READ       = 3,    /**< Failed to read EFL                   */
    MD_ERROR_EFL_ERASE      = 4,    /**< Failed to erase EFL sector           */
    MD_ERROR_EFL_PROGRAM    = 5,    /**< Failed to program EFL                */
    MD_ERROR_EFL_STOP       = 6,    /**< Failed to stop EFL driver            */
    MD_ERROR_CRC            = 7,    /**< CRC validation failed                */
    MD_ERROR_STORAGE        = 8,    /**< Storage validation failed            */
} md_result_t;

/**
 * @brief   Metadata data structure.
 * @note    This is the data stored in EFL page 0. Only contains
 *          production-programmed values that must survive firmware updates.
 */
typedef struct {
    char serial_number[MD_SERIAL_NUMBER_SIZE];   /**< Unique serial number    */
    /* Add future production-programmed fields here */
} md_data_t;

/*===========================================================================*/
/* Module public functions.                                                  */
/*===========================================================================*/

/**
 * @name    Initialization
 * @{
 */

/**
 * @brief   Initialize the device metadata module.
 * @note    Loads metadata from EFL page 0. If the data is invalid or
 *          uninitialized, factory defaults are loaded into the RAM cache
 *          (but not written to flash — production programming writes flash).
 *
 * @return  Result code indicating success or failure reason.
 */
md_result_t device_metadata_init(void);

/** @} */

/**
 * @name    Data access
 * @{
 */

/**
 * @brief   Get pointer to the metadata structure.
 * @note    Returns a const pointer. Metadata is read-only.
 * @pre     device_metadata_init() must have been called.
 *
 * @return  Pointer to the metadata structure, or NULL if not initialized.
 */
const md_data_t *device_metadata_get(void);

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
const char *device_metadata_result_str(md_result_t result);

/**
 * @brief   Check if the module is initialized.
 *
 * @return  true if initialized, false otherwise.
 */
bool device_metadata_is_initialized(void);

/** @} */

#endif /* DEVICE_METADATA_H */

/** @} */
