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
 * @brief   Persistent data storage module for device configuration.
 * @note    This module provides type-safe access to persistent data stored
 *          in EFL (Embedded Flash) with support for field-level access control
 *          for USB/external access.
 *
 * @details The module uses a hybrid approach:
 *          - Type-safe struct for internal application access
 *          - Field registry with metadata for USB/external access control
 *          - CRC32 integrity checking
 *          - Factory default restoration
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
#define PD_SERIAL_NUMBER_SIZE   16U     /**< Serial number field size (bytes) */
/** @} */

/**
 * @name    Storage format constants
 * @{
 */
#define PD_MAGIC                0x50444154U /**< Magic number "PDAT"          */
#define PD_VERSION              0x0001U     /**< Data format version          */
/** @} */

/*===========================================================================*/
/* Module data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Access level for persistent data fields.
 * @note    Used to control external (USB) access to fields.
 */
typedef enum {
    PD_ACCESS_INTERNAL = 0,     /**< Only readable internally by device       */
    PD_ACCESS_RO       = 1,     /**< Read-only over USB                       */
    PD_ACCESS_RW       = 2,     /**< Read-write over USB                      */
} pd_access_t;

/**
 * @brief   Field identifiers for external access.
 * @note    Used by USB handler to identify fields for read/write operations.
 */
typedef enum {
    PD_FIELD_DEVICE_NAME    = 0x01,     /**< Device name field                */
    PD_FIELD_SERIAL_NUMBER  = 0x02,     /**< Serial number field              */
    /* Add future fields here with sequential IDs */
} pd_field_id_t;

/**
 * @brief   Result codes for persistent data operations.
 */
typedef enum {
    PD_OK                   = 0,    /**< Operation successful                 */
    PD_ERROR_NOT_INIT       = 1,    /**< Module not initialized               */
    PD_ERROR_INVALID_FIELD  = 2,    /**< Invalid field ID                     */
    PD_ERROR_ACCESS_DENIED  = 3,    /**< Access level insufficient            */
    PD_ERROR_BUFFER_SIZE    = 4,    /**< Buffer too small                     */
    PD_ERROR_EFL_START      = 5,    /**< Failed to start EFL driver           */
    PD_ERROR_EFL_ERASE      = 6,    /**< Failed to erase EFL sector           */
    PD_ERROR_EFL_PROGRAM    = 7,    /**< Failed to program EFL                */
    PD_ERROR_EFL_READ       = 8,    /**< Failed to read EFL                   */
    PD_ERROR_EFL_STOP       = 9,    /**< Failed to stop EFL driver            */
    PD_ERROR_CRC            = 10,   /**< CRC validation failed                */
    PD_ERROR_STORAGE        = 11,   /**< Storage validation failed            */
} pd_result_t;

/**
 * @brief   Persistent data structure.
 * @note    This is the actual data stored in EFL. Fields are accessed
 *          directly for internal use, or via field ID for external access.
 */
typedef struct {
    char device_name[PD_DEVICE_NAME_SIZE];      /**< Product/device name      */
    char serial_number[PD_SERIAL_NUMBER_SIZE];  /**< Unique serial number     */
    /* Add future data fields here */
} pd_data_t;

/**
 * @brief   Field metadata structure for registry.
 * @note    Used to provide field information for USB enumeration and
 *          access control.
 */
typedef struct {
    pd_field_id_t   id;         /**< Field identifier                         */
    uint16_t        offset;     /**< Offset within pd_data_t                  */
    uint16_t        size;       /**< Field size in bytes                      */
    pd_access_t     access;     /**< Access level                             */
    const char     *name;       /**< Human-readable field name                */
} pd_field_info_t;

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

/*===========================================================================*/
/* Module inline functions.                                                  */
/*===========================================================================*/

/*===========================================================================*/
/* Module public functions.                                                  */
/*===========================================================================*/

/**
 * @name    Initialization functions
 * @{
 */

/**
 * @brief   Initialize the persistent data module.
 * @note    This function loads data from EFL storage. If the data is invalid
 *          or uninitialized, factory defaults are loaded automatically.
 *
 * @return  Result code indicating success or failure reason.
 * @retval  PD_OK               Initialization successful.
 * @retval  PD_ERROR_STORAGE    EFL storage validation failed.
 * @retval  PD_ERROR_EFL_*      EFL driver error.
 */
pd_result_t persistent_data_init(void);

/** @} */

/**
 * @name    Direct data access (internal use)
 * @{
 */

/**
 * @brief   Get pointer to the persistent data structure.
 * @note    Returns a const pointer to prevent accidental modification.
 *          Use persistent_data_write_field() to modify data.
 * @pre     persistent_data_init() must have been called.
 *
 * @return  Pointer to the persistent data structure, or NULL if not initialized.
 */
const pd_data_t *persistent_data_get(void);

/** @} */

/**
 * @name    Field-based access (for USB/external use)
 * @{
 */

/**
 * @brief   Read a field by ID.
 * @note    Checks access level before allowing read.
 * @pre     persistent_data_init() must have been called.
 *
 * @param[in]     id        Field identifier.
 * @param[out]    buffer    Buffer to store field data.
 * @param[in,out] size      On input: buffer size. On output: actual data size.
 *
 * @return  Result code indicating success or failure reason.
 * @retval  PD_OK                   Read successful.
 * @retval  PD_ERROR_NOT_INIT       Module not initialized.
 * @retval  PD_ERROR_INVALID_FIELD  Unknown field ID.
 * @retval  PD_ERROR_ACCESS_DENIED  Field is INTERNAL access only.
 * @retval  PD_ERROR_BUFFER_SIZE    Buffer too small (size updated with required).
 */
pd_result_t persistent_data_read_field(pd_field_id_t id, void *buffer, size_t *size);

/**
 * @brief   Write a field by ID.
 * @note    Checks access level (must be RW) before allowing write.
 *          Changes are held in RAM until persistent_data_save() is called.
 * @pre     persistent_data_init() must have been called.
 *
 * @param[in] id    Field identifier.
 * @param[in] data  Data to write.
 * @param[in] size  Size of data.
 *
 * @return  Result code indicating success or failure reason.
 * @retval  PD_OK                   Write successful (to RAM cache).
 * @retval  PD_ERROR_NOT_INIT       Module not initialized.
 * @retval  PD_ERROR_INVALID_FIELD  Unknown field ID.
 * @retval  PD_ERROR_ACCESS_DENIED  Field is not RW access.
 * @retval  PD_ERROR_BUFFER_SIZE    Data size exceeds field size.
 */
pd_result_t persistent_data_write_field(pd_field_id_t id, const void *data, size_t size);

/**
 * @brief   Get the access level for a field.
 * @pre     persistent_data_init() must have been called.
 *
 * @param[in] id  Field identifier.
 *
 * @return  Access level, or PD_ACCESS_INTERNAL if field ID is invalid.
 */
pd_access_t persistent_data_get_access(pd_field_id_t id);

/** @} */

/**
 * @name    Field registry enumeration
 * @{
 */

/**
 * @brief   Get the number of registered fields.
 *
 * @return  Number of fields in the registry.
 */
size_t persistent_data_field_count(void);

/**
 * @brief   Get information about a field by index.
 * @note    Used to enumerate all fields (e.g., for USB listing).
 *
 * @param[in]  index    Field index (0 to field_count-1).
 * @param[out] info     Pointer to store field information (can be NULL).
 *
 * @return  true if index is valid, false otherwise.
 */
bool persistent_data_get_field_info(size_t index, pd_field_info_t *info);

/**
 * @brief   Find field information by ID.
 *
 * @param[in]  id       Field identifier to search for.
 * @param[out] info     Pointer to store field information (can be NULL).
 *
 * @return  true if field found, false otherwise.
 */
bool persistent_data_find_field(pd_field_id_t id, pd_field_info_t *info);

/** @} */

/**
 * @name    Persistence operations
 * @{
 */

/**
 * @brief   Save current data to EFL storage.
 * @note    Writes the RAM cache to flash with updated CRC.
 * @pre     persistent_data_init() must have been called.
 *
 * @return  Result code indicating success or failure reason.
 * @retval  PD_OK               Save successful.
 * @retval  PD_ERROR_NOT_INIT   Module not initialized.
 * @retval  PD_ERROR_EFL_*      EFL driver error.
 */
pd_result_t persistent_data_save(void);

/**
 * @brief   Reload data from EFL storage.
 * @note    Discards any unsaved changes in RAM cache.
 * @pre     persistent_data_init() must have been called.
 *
 * @return  Result code indicating success or failure reason.
 * @retval  PD_OK               Load successful.
 * @retval  PD_ERROR_NOT_INIT   Module not initialized.
 * @retval  PD_ERROR_CRC        Stored data is invalid (defaults loaded).
 * @retval  PD_ERROR_EFL_*      EFL driver error.
 */
pd_result_t persistent_data_load(void);

/**
 * @brief   Reset all data to factory defaults.
 * @note    Loads defaults into RAM cache. Call persistent_data_save()
 *          to persist the reset to flash.
 * @pre     persistent_data_init() must have been called.
 *
 * @return  Result code indicating success or failure reason.
 * @retval  PD_OK               Reset successful (in RAM).
 * @retval  PD_ERROR_NOT_INIT   Module not initialized.
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
