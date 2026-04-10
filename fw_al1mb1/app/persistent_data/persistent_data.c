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
 * @brief   Persistent settings storage module implementation.
 * @note    Uses EFL page 1 for user-configurable settings. Metadata lives
 *          on EFL page 0 (see device_metadata.c).
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
 */
typedef struct {
    uint32_t    magic;          /**< Magic number (PD_MAGIC)                  */
    uint16_t    version;        /**< Data format version                      */
    uint16_t    reserved;       /**< Reserved for alignment/future use        */
} pd_header_t;

/**
 * @brief   Complete storage structure in EFL.
 */
typedef struct {
    pd_header_t header;         /**< Header for validation                    */
    pd_data_t   data;           /**< Settings data                            */
    uint32_t    crc;            /**< CRC32 of header + data                   */
} pd_storage_t;

/**
 * @brief   EFL offset for settings storage (page 1 of EFL region).
 * @note    Page 0 is used by device_metadata. Settings start at page 1.
 */
#define PD_EFL_OFFSET   (EFL_STORAGE_OFFSET + EFL_PAGE_SIZE)

/**
 * @brief   EFL sector for settings storage (page 1).
 */
#define PD_EFL_SECTOR   (EFL_STORAGE_FIRST_SECTOR + 1U)

/*===========================================================================*/
/* Module local variables.                                                   */
/*===========================================================================*/

/**
 * @brief   Module initialization state.
 */
static bool pd_initialized = false;

/**
 * @brief   RAM cache of settings data.
 */
static pd_data_t pd_cache;

/**
 * @brief   Result code strings.
 */
static const char * const pd_result_strings[] = {
    [PD_OK]                  = "OK",
    [PD_ERROR_NOT_INIT]      = "Not initialized",
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
    strncpy(pd_cache.device_name, PD_DEFAULT_DEVICE_NAME,
            PD_DEVICE_NAME_SIZE - 1);
    pd_cache.device_name[PD_DEVICE_NAME_SIZE - 1] = '\0';
    pd_cache.log_level = PD_DEFAULT_LOG_LEVEL;
}

/**
 * @brief   Read settings from EFL storage (page 1).
 *
 * @param[out] storage  Buffer to store read data.
 *
 * @return  Result code.
 */
static pd_result_t pd_efl_read(pd_storage_t *storage) {
    flash_error_t err;

    eflStart(&EFLD1, NULL);

    err = flashRead(&EFLD1, (flash_offset_t)PD_EFL_OFFSET,
                    sizeof(pd_storage_t), (uint8_t *)storage);

    eflStop(&EFLD1);

    if (err != FLASH_NO_ERROR) {
        return PD_ERROR_EFL_READ;
    }

    return PD_OK;
}

/**
 * @brief   Write settings to EFL storage (page 1).
 *
 * @param[in] storage  Data to write.
 *
 * @return  Result code.
 */
static pd_result_t pd_efl_write(const pd_storage_t *storage) {
    flash_error_t err;

    eflStart(&EFLD1, NULL);

    /* Erase settings page (page 1) */
    err = flashStartEraseSector(&EFLD1, PD_EFL_SECTOR);
    if (err != FLASH_NO_ERROR) {
        eflStop(&EFLD1);
        return PD_ERROR_EFL_ERASE;
    }

    err = flashWaitErase((BaseFlash *)&EFLD1);
    if (err != FLASH_NO_ERROR) {
        eflStop(&EFLD1);
        return PD_ERROR_EFL_ERASE;
    }

    err = flashVerifyErase(&EFLD1, PD_EFL_SECTOR);
    if (err != FLASH_NO_ERROR) {
        eflStop(&EFLD1);
        return PD_ERROR_EFL_ERASE;
    }

    /* Program data */
    err = flashProgram(&EFLD1, (flash_offset_t)PD_EFL_OFFSET,
                       sizeof(pd_storage_t), (const uint8_t *)storage);
    if (err != FLASH_NO_ERROR) {
        eflStop(&EFLD1);
        return PD_ERROR_EFL_PROGRAM;
    }

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
    if (storage->header.magic != PD_MAGIC) {
        return false;
    }

    if (storage->header.version != PD_VERSION) {
        return false;
    }

    uint32_t calc_crc = pd_crc32(storage, offsetof(pd_storage_t, crc));

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

    if (!efl_storage_validate()) {
        return PD_ERROR_STORAGE;
    }

    result = pd_efl_read(&storage);
    if (result != PD_OK) {
        /* EFL read failed, load defaults */
        pd_load_defaults();
        pd_initialized = true;
        return result;
    }

    if (!pd_validate_storage(&storage)) {
        /* Invalid data (first boot or format change), load defaults */
        pd_load_defaults();
        pd_initialized = true;
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

pd_result_t persistent_data_set_device_name(const char *name) {
    if (!pd_initialized) {
        return PD_ERROR_NOT_INIT;
    }

    size_t len = strlen(name);
    if (len >= PD_DEVICE_NAME_SIZE) {
        return PD_ERROR_BUFFER_SIZE;
    }

    memset(pd_cache.device_name, 0, PD_DEVICE_NAME_SIZE);
    memcpy(pd_cache.device_name, name, len + 1);

    return PD_OK;
}

pd_result_t persistent_data_set_log_level(uint8_t level) {
    if (!pd_initialized) {
        return PD_ERROR_NOT_INIT;
    }

    if (level > 4) {
        return PD_ERROR_BUFFER_SIZE;
    }

    pd_cache.log_level = level;

    return PD_OK;
}

pd_result_t persistent_data_save(void) {
    pd_storage_t storage;

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

    /* Write to EFL page 1 */
    return pd_efl_write(&storage);
}

pd_result_t persistent_data_load(void) {
    pd_storage_t storage;
    pd_result_t result;

    if (!pd_initialized) {
        return PD_ERROR_NOT_INIT;
    }

    result = pd_efl_read(&storage);
    if (result != PD_OK) {
        return result;
    }

    if (!pd_validate_storage(&storage)) {
        pd_load_defaults();
        return PD_ERROR_CRC;
    }

    memcpy(&pd_cache, &storage.data, sizeof(pd_data_t));

    return PD_OK;
}

pd_result_t persistent_data_factory_reset(void) {
    if (!pd_initialized) {
        return PD_ERROR_NOT_INIT;
    }

    pd_load_defaults();

    /* Save defaults to flash immediately */
    return persistent_data_save();
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
