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
 * @file    device_metadata.c
 * @brief   Read-only device metadata storage module implementation.
 * @note    Uses EFL page 0 for production-programmed metadata (serial number).
 *          Physically separated from settings (EFL page 1) so that factory
 *          resets never touch metadata.
 *
 * @addtogroup DEVICE_METADATA
 * @{
 */

#include "device_metadata.h"
#include "hal.h"
#include "efl_storage.h"
#include <string.h>
#include <stddef.h>

/*===========================================================================*/
/* Module local definitions.                                                 */
/*===========================================================================*/

/**
 * @brief   Storage header structure.
 */
typedef struct {
    uint32_t    magic;          /**< Magic number (MD_MAGIC)                  */
    uint16_t    version;        /**< Data format version                      */
    uint16_t    reserved;       /**< Reserved for alignment/future use        */
} md_header_t;

/**
 * @brief   Complete storage structure in EFL.
 * @note    Stored at the start of EFL page 0.
 */
typedef struct {
    md_header_t header;         /**< Header for validation                    */
    md_data_t   data;           /**< Metadata fields                          */
    uint32_t    crc;            /**< CRC32 of header + data                   */
} md_storage_t;

/**
 * @brief   EFL offset for metadata storage (page 0 of EFL region).
 */
#define MD_EFL_OFFSET   EFL_STORAGE_OFFSET

/**
 * @brief   EFL sector for metadata storage (page 0).
 */
#define MD_EFL_SECTOR   EFL_STORAGE_FIRST_SECTOR

/*===========================================================================*/
/* Module local variables.                                                   */
/*===========================================================================*/

/**
 * @brief   Module initialization state.
 */
static bool md_initialized = false;

/**
 * @brief   RAM cache of metadata.
 */
static md_data_t md_cache;

/**
 * @brief   Result code strings.
 */
static const char * const md_result_strings[] = {
    [MD_OK]                 = "OK",
    [MD_ERROR_NOT_INIT]     = "Not initialized",
    [MD_ERROR_EFL_START]    = "EFL start failed",
    [MD_ERROR_EFL_READ]     = "EFL read failed",
    [MD_ERROR_EFL_ERASE]    = "EFL erase failed",
    [MD_ERROR_EFL_PROGRAM]  = "EFL program failed",
    [MD_ERROR_EFL_STOP]     = "EFL stop failed",
    [MD_ERROR_CRC]          = "CRC validation failed",
    [MD_ERROR_STORAGE]      = "Storage validation failed",
};

/*===========================================================================*/
/* Module local functions.                                                   */
/*===========================================================================*/

/**
 * @brief   CRC32 calculation (IEEE 802.3 polynomial).
 *
 * @param[in] data  Pointer to data.
 * @param[in] len   Length of data in bytes.
 *
 * @return  CRC32 value.
 */
static uint32_t md_crc32(const void *data, size_t len) {
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
 * @brief   Load default metadata values into RAM cache.
 */
static void md_load_defaults(void) {
    memset(&md_cache, 0, sizeof(md_cache));
    strncpy(md_cache.serial_number, MD_DEFAULT_SERIAL_NUMBER,
            MD_SERIAL_NUMBER_SIZE - 1);
    md_cache.serial_number[MD_SERIAL_NUMBER_SIZE - 1] = '\0';
}

/**
 * @brief   Read metadata from EFL storage.
 *
 * @param[out] storage  Buffer to store read data.
 *
 * @return  Result code.
 */
static md_result_t md_efl_read(md_storage_t *storage) {
    flash_error_t err;

    eflStart(&EFLD1, NULL);

    err = flashRead(&EFLD1, (flash_offset_t)MD_EFL_OFFSET,
                    sizeof(md_storage_t), (uint8_t *)storage);

    eflStop(&EFLD1);

    if (err != FLASH_NO_ERROR) {
        return MD_ERROR_EFL_READ;
    }

    return MD_OK;
}

/**
 * @brief   Validate storage data integrity.
 *
 * @param[in] storage  Storage data to validate.
 *
 * @return  true if valid, false otherwise.
 */
static bool md_validate_storage(const md_storage_t *storage) {
    if (storage->header.magic != MD_MAGIC) {
        return false;
    }

    if (storage->header.version != MD_VERSION) {
        return false;
    }

    uint32_t calc_crc = md_crc32(storage, offsetof(md_storage_t, crc));

    if (calc_crc != storage->crc) {
        return false;
    }

    return true;
}

/*===========================================================================*/
/* Module exported functions.                                                */
/*===========================================================================*/

md_result_t device_metadata_init(void) {
    md_storage_t storage;
    md_result_t result;

    if (!efl_storage_validate()) {
        return MD_ERROR_STORAGE;
    }

    result = md_efl_read(&storage);
    if (result != MD_OK) {
        /* EFL read failed, load defaults */
        md_load_defaults();
        md_initialized = true;
        return result;
    }

    if (!md_validate_storage(&storage)) {
        /* Invalid data (first boot or no production programming yet) */
        md_load_defaults();
        md_initialized = true;
        return MD_OK;
    }

    /* Copy valid data to cache */
    memcpy(&md_cache, &storage.data, sizeof(md_data_t));
    md_initialized = true;

    return MD_OK;
}

const md_data_t *device_metadata_get(void) {
    if (!md_initialized) {
        return NULL;
    }
    return &md_cache;
}

const char *device_metadata_result_str(md_result_t result) {
    if ((size_t)result < sizeof(md_result_strings) / sizeof(md_result_strings[0])) {
        return md_result_strings[result];
    }
    return "Unknown error";
}

bool device_metadata_is_initialized(void) {
    return md_initialized;
}

/** @} */
