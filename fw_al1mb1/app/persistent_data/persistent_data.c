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
 * @file    persistent_data.c
 * @brief   Persistent data storage module implementation.
 *
 * @addtogroup PERSISTENT_DATA
 * @{
 */

#include "persistent_data.h"
#include "persistent_data_defaults.h"
#include "hal.h"
#include "efl_storage.h"
#include <string.h>
#include <stddef.h>

/*===========================================================================*/
/* Module local definitions.                                                 */
/*===========================================================================*/

/**
 * @brief   Storage header structure.
 * @note    Stored at the beginning of EFL region for validation.
 */
typedef struct {
    uint32_t    magic;          /**< Magic number (PD_MAGIC)                  */
    uint16_t    version;        /**< Data format version                      */
    uint16_t    reserved;       /**< Reserved for alignment/future use        */
} pd_header_t;

/**
 * @brief   Complete storage structure in EFL.
 * @note    This is the actual layout written to flash.
 */
typedef struct {
    pd_header_t header;         /**< Header for validation                    */
    pd_data_t   data;           /**< User data                                */
    uint32_t    crc;            /**< CRC32 of header + data                   */
} pd_storage_t;

/*===========================================================================*/
/* Module local variables.                                                   */
/*===========================================================================*/

/**
 * @brief   Module initialization state.
 */
static bool pd_initialized = false;

/**
 * @brief   RAM cache of persistent data.
 */
static pd_data_t pd_cache;

/**
 * @brief   Field registry for metadata and access control.
 * @note    Order matters for enumeration; keep in logical order.
 */
static const pd_field_info_t pd_fields[] = {
    {
        .id     = PD_FIELD_DEVICE_NAME,
        .offset = offsetof(pd_data_t, device_name),
        .size   = PD_DEVICE_NAME_SIZE,
        .access = PD_ACCESS_RW,
        .name   = "device_name"
    },
    {
        .id     = PD_FIELD_SERIAL_NUMBER,
        .offset = offsetof(pd_data_t, serial_number),
        .size   = PD_SERIAL_NUMBER_SIZE,
        .access = PD_ACCESS_RO,
        .name   = "serial_number"
    },
    /* Add future fields here */
};

/**
 * @brief   Number of fields in the registry.
 */
#define PD_FIELD_COUNT  (sizeof(pd_fields) / sizeof(pd_fields[0]))

/**
 * @brief   Result code strings.
 */
static const char * const pd_result_strings[] = {
    [PD_OK]                  = "OK",
    [PD_ERROR_NOT_INIT]      = "Not initialized",
    [PD_ERROR_INVALID_FIELD] = "Invalid field ID",
    [PD_ERROR_ACCESS_DENIED] = "Access denied",
    [PD_ERROR_BUFFER_SIZE]   = "Buffer size error",
    [PD_ERROR_EFL_START]     = "EFL start failed",
    [PD_ERROR_EFL_ERASE]     = "EFL erase failed",
    [PD_ERROR_EFL_PROGRAM]   = "EFL program failed",
    [PD_ERROR_EFL_READ]      = "EFL read failed",
    [PD_ERROR_EFL_STOP]      = "EFL stop failed",
    [PD_ERROR_CRC]           = "CRC validation failed",
    [PD_ERROR_STORAGE]       = "Storage validation failed",
};

/*===========================================================================*/
/* Module local functions.                                                   */
/*===========================================================================*/

/**
 * @brief   CRC32 calculation (IEEE 802.3 polynomial).
 * @note    Simple byte-at-a-time implementation, suitable for small data.
 *
 * @param[in] data  Pointer to data.
 * @param[in] len   Length of data in bytes.
 *
 * @return  CRC32 value.
 */
static uint32_t pd_crc32(const void *data, size_t len) {
    const uint8_t *p = (const uint8_t *)data;
    uint32_t crc = 0xFFFFFFFFU;

    while (len--) {
        crc ^= *p++;
        for (int i = 0; i < 8; i++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320U;
            } else {
                crc >>= 1;
            }
        }
    }

    return ~crc;
}

/**
 * @brief   Load factory default values into RAM cache.
 */
static void pd_load_defaults(void) {
    memset(&pd_cache, 0, sizeof(pd_cache));
    strncpy(pd_cache.device_name, PD_DEFAULT_DEVICE_NAME, PD_DEVICE_NAME_SIZE - 1);
    pd_cache.device_name[PD_DEVICE_NAME_SIZE - 1] = '\0';
    strncpy(pd_cache.serial_number, PD_DEFAULT_SERIAL_NUMBER, PD_SERIAL_NUMBER_SIZE - 1);
    pd_cache.serial_number[PD_SERIAL_NUMBER_SIZE - 1] = '\0';
}

/**
 * @brief   Find field info by ID.
 *
 * @param[in] id  Field identifier.
 *
 * @return  Pointer to field info, or NULL if not found.
 */
static const pd_field_info_t *pd_find_field_internal(pd_field_id_t id) {
    for (size_t i = 0; i < PD_FIELD_COUNT; i++) {
        if (pd_fields[i].id == id) {
            return &pd_fields[i];
        }
    }
    return NULL;
}

/**
 * @brief   Read data from EFL storage.
 *
 * @param[out] storage  Buffer to store read data.
 *
 * @return  Result code.
 */
static pd_result_t pd_efl_read(pd_storage_t *storage) {
    flash_error_t err;

    /* Start EFL driver */
    eflStart(&EFLD1, NULL);

    /* Read storage structure from EFL */
    err = flashRead(&EFLD1, (flash_offset_t)EFL_STORAGE_OFFSET,
                    sizeof(pd_storage_t), (uint8_t *)storage);
    
    /* Stop EFL driver */
    eflStop(&EFLD1);

    if (err != FLASH_NO_ERROR) {
        return PD_ERROR_EFL_READ;
    }

    return PD_OK;
}

/**
 * @brief   Write data to EFL storage.
 *
 * @param[in] storage  Data to write.
 *
 * @return  Result code.
 */
static pd_result_t pd_efl_write(const pd_storage_t *storage) {
    flash_error_t err;
    flash_sector_t sector;

    /* Start EFL driver */
    eflStart(&EFLD1, NULL);

    /* Get sector number for EFL storage region */
    sector = EFL_STORAGE_FIRST_SECTOR;

    /* Erase sector */
    err = flashStartEraseSector(&EFLD1, sector);
    if (err != FLASH_NO_ERROR) {
        eflStop(&EFLD1);
        return PD_ERROR_EFL_ERASE;
    }

    /* Wait for erase to complete */
    err = flashWaitErase((BaseFlash *)&EFLD1);
    if (err != FLASH_NO_ERROR) {
        eflStop(&EFLD1);
        return PD_ERROR_EFL_ERASE;
    }

    /* Verify erase */
    err = flashVerifyErase(&EFLD1, sector);
    if (err != FLASH_NO_ERROR) {
        eflStop(&EFLD1);
        return PD_ERROR_EFL_ERASE;
    }

    /* Program data */
    err = flashProgram(&EFLD1, (flash_offset_t)EFL_STORAGE_OFFSET,
                       sizeof(pd_storage_t), (const uint8_t *)storage);
    if (err != FLASH_NO_ERROR) {
        eflStop(&EFLD1);
        return PD_ERROR_EFL_PROGRAM;
    }

    /* Stop EFL driver */
    eflStop(&EFLD1);

    return PD_OK;
}

/**
 * @brief   Validate storage data integrity.
 *
 * @param[in] storage  Storage data to validate.
 *
 * @return  true if valid, false otherwise.
 */
static bool pd_validate_storage(const pd_storage_t *storage) {
    uint32_t calc_crc;

    /* Check magic number */
    if (storage->header.magic != PD_MAGIC) {
        return false;
    }

    /* Check version (accept current version only for now) */
    if (storage->header.version != PD_VERSION) {
        return false;
    }

    /* Calculate CRC over header + data */
    calc_crc = pd_crc32(storage, offsetof(pd_storage_t, crc));

    /* Compare with stored CRC */
    if (calc_crc != storage->crc) {
        return false;
    }

    return true;
}

/*===========================================================================*/
/* Module exported functions.                                                */
/*===========================================================================*/

pd_result_t persistent_data_init(void) {
    pd_storage_t storage;
    pd_result_t result;

    /* Validate EFL storage configuration */
    if (!efl_storage_validate()) {
        return PD_ERROR_STORAGE;
    }

    /* Try to read existing data from EFL */
    result = pd_efl_read(&storage);
    if (result != PD_OK) {
        /* EFL read failed, load defaults */
        pd_load_defaults();
        pd_initialized = true;
        return result;
    }

    /* Validate the read data */
    if (!pd_validate_storage(&storage)) {
        /* Invalid data (first boot or corruption), load defaults */
        pd_load_defaults();
        pd_initialized = true;
        /* Return OK since we successfully initialized with defaults */
        return PD_OK;
    }

    /* Copy valid data to cache */
    memcpy(&pd_cache, &storage.data, sizeof(pd_data_t));
    pd_initialized = true;

    return PD_OK;
}

const pd_data_t *persistent_data_get(void) {
    if (!pd_initialized) {
        return NULL;
    }
    return &pd_cache;
}

pd_result_t persistent_data_read_field(pd_field_id_t id, void *buffer, size_t *size) {
    const pd_field_info_t *field;

    if (!pd_initialized) {
        return PD_ERROR_NOT_INIT;
    }

    field = pd_find_field_internal(id);
    if (field == NULL) {
        return PD_ERROR_INVALID_FIELD;
    }

    /* Check access level (INTERNAL fields cannot be read externally) */
    if (field->access == PD_ACCESS_INTERNAL) {
        return PD_ERROR_ACCESS_DENIED;
    }

    /* Check buffer size */
    if (*size < field->size) {
        *size = field->size;
        return PD_ERROR_BUFFER_SIZE;
    }

    /* Copy field data to buffer */
    memcpy(buffer, (const uint8_t *)&pd_cache + field->offset, field->size);
    *size = field->size;

    return PD_OK;
}

pd_result_t persistent_data_write_field(pd_field_id_t id, const void *data, size_t size) {
    const pd_field_info_t *field;

    if (!pd_initialized) {
        return PD_ERROR_NOT_INIT;
    }

    field = pd_find_field_internal(id);
    if (field == NULL) {
        return PD_ERROR_INVALID_FIELD;
    }

    /* Check access level (only RW fields can be written) */
    if (field->access != PD_ACCESS_RW) {
        return PD_ERROR_ACCESS_DENIED;
    }

    /* Check data size */
    if (size > field->size) {
        return PD_ERROR_BUFFER_SIZE;
    }

    /* Copy data to cache (zero-pad if smaller) */
    memset((uint8_t *)&pd_cache + field->offset, 0, field->size);
    memcpy((uint8_t *)&pd_cache + field->offset, data, size);

    return PD_OK;
}

pd_access_t persistent_data_get_access(pd_field_id_t id) {
    const pd_field_info_t *field = pd_find_field_internal(id);
    if (field == NULL) {
        return PD_ACCESS_INTERNAL;
    }
    return field->access;
}

size_t persistent_data_field_count(void) {
    return PD_FIELD_COUNT;
}

bool persistent_data_get_field_info(size_t index, pd_field_info_t *info) {
    if (index >= PD_FIELD_COUNT) {
        return false;
    }

    if (info != NULL) {
        *info = pd_fields[index];
    }

    return true;
}

bool persistent_data_find_field(pd_field_id_t id, pd_field_info_t *info) {
    const pd_field_info_t *field = pd_find_field_internal(id);
    if (field == NULL) {
        return false;
    }

    if (info != NULL) {
        *info = *field;
    }

    return true;
}

bool persistent_data_find_field_by_name(const char *name, pd_field_info_t *info) {
    if (name == NULL) {
        return false;
    }

    for (size_t i = 0; i < PD_FIELD_COUNT; i++) {
        if (strcmp(pd_fields[i].name, name) == 0) {
            if (info != NULL) {
                *info = pd_fields[i];
            }
            return true;
        }
    }

    return false;
}

pd_result_t persistent_data_save(void) {
    pd_storage_t storage;
    pd_result_t result;

    if (!pd_initialized) {
        return PD_ERROR_NOT_INIT;
    }

    /* Prepare storage structure */
    storage.header.magic = PD_MAGIC;
    storage.header.version = PD_VERSION;
    storage.header.reserved = 0;
    memcpy(&storage.data, &pd_cache, sizeof(pd_data_t));

    /* Calculate CRC */
    storage.crc = pd_crc32(&storage, offsetof(pd_storage_t, crc));

    /* Write to EFL */
    result = pd_efl_write(&storage);

    return result;
}

pd_result_t persistent_data_load(void) {
    pd_storage_t storage;
    pd_result_t result;

    if (!pd_initialized) {
        return PD_ERROR_NOT_INIT;
    }

    /* Read from EFL */
    result = pd_efl_read(&storage);
    if (result != PD_OK) {
        return result;
    }

    /* Validate */
    if (!pd_validate_storage(&storage)) {
        pd_load_defaults();
        return PD_ERROR_CRC;
    }

    /* Update cache */
    memcpy(&pd_cache, &storage.data, sizeof(pd_data_t));

    return PD_OK;
}

pd_result_t persistent_data_factory_reset(void) {
    if (!pd_initialized) {
        return PD_ERROR_NOT_INIT;
    }

    pd_load_defaults();

    return PD_OK;
}

const char *persistent_data_result_str(pd_result_t result) {
    if ((size_t)result < sizeof(pd_result_strings) / sizeof(pd_result_strings[0])) {
        return pd_result_strings[result];
    }
    return "Unknown error";
}

bool persistent_data_is_initialized(void) {
    return pd_initialized;
}

/** @} */
