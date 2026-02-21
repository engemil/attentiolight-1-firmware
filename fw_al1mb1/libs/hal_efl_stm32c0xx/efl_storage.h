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
 * @file    efl_storage.h
 * @brief   EFL storage region helper macros using linker symbols.
 * @note    These macros provide a portable way to access the EFL storage
 *          region defined in the linker script. The actual addresses are
 *          exported as linker symbols, not hardcoded values.
 *
 * @addtogroup EFL_STORAGE
 * @{
 */

#ifndef EFL_STORAGE_H
#define EFL_STORAGE_H

#include <stdint.h>
#include <stddef.h>

/*===========================================================================*/
/* External linker symbols.                                                  */
/*===========================================================================*/

/**
 * @brief   Linker symbols for EFL storage region.
 * @note    These symbols are defined in the linker script (_efl variant).
 *          They mark the boundaries of the reserved EFL storage area.
 */
extern uint8_t __eflash_base__[];
extern uint8_t __eflash_end__[];
extern uint8_t __eflash_size__[];

/*===========================================================================*/
/* Helper macros.                                                            */
/*===========================================================================*/

/**
 * @brief   Get the base address of the EFL storage region.
 * @return  Pointer to the start of the EFL storage region.
 */
#define EFL_STORAGE_BASE        ((uint8_t *)__eflash_base__)

/**
 * @brief   Get the end address of the EFL storage region.
 * @return  Pointer to the end of the EFL storage region (exclusive).
 */
#define EFL_STORAGE_END         ((uint8_t *)__eflash_end__)

/**
 * @brief   Get the size of the EFL storage region in bytes.
 * @return  Size of the EFL storage region.
 */
#define EFL_STORAGE_SIZE        ((size_t)__eflash_size__)

/**
 * @brief   STM32C0xx flash page size (2KB for all variants).
 */
#define EFL_PAGE_SIZE           2048U

/**
 * @brief   Number of pages in the EFL storage region.
 * @note    Default is 4 pages (8KB total) but actual value depends on
 *          linker script configuration.
 */
#define EFL_STORAGE_PAGES       (EFL_STORAGE_SIZE / EFL_PAGE_SIZE)

/**
 * @brief   Calculate the flash offset from FLASH_BASE to EFL storage.
 * @return  Offset in bytes from FLASH_BASE to the EFL storage region.
 */
#define EFL_STORAGE_OFFSET      ((size_t)(EFL_STORAGE_BASE - (uint8_t *)0x08000000U))

/**
 * @brief   Calculate the first sector number of the EFL storage region.
 * @return  Sector number (page index) of the first EFL storage page.
 */
#define EFL_STORAGE_FIRST_SECTOR    (EFL_STORAGE_OFFSET / EFL_PAGE_SIZE)

/**
 * @brief   Check if an address is within the EFL storage region.
 * @param[in] addr  Address to check.
 * @return    true if address is within EFL storage, false otherwise.
 */
#define EFL_STORAGE_CONTAINS(addr)  \
    (((uint8_t *)(addr) >= EFL_STORAGE_BASE) && ((uint8_t *)(addr) < EFL_STORAGE_END))

/**
 * @brief   Convert a storage-relative offset to an absolute address.
 * @param[in] offset  Offset from start of EFL storage region.
 * @return    Pointer to the absolute address.
 */
#define EFL_STORAGE_ADDR(offset)    (EFL_STORAGE_BASE + (offset))

/**
 * @brief   Get the sector number for an address in EFL storage.
 * @param[in] addr  Address within EFL storage region.
 * @return    Sector number (page index from FLASH_BASE).
 */
#define EFL_STORAGE_SECTOR(addr)    \
    (((size_t)((uint8_t *)(addr) - (uint8_t *)0x08000000U)) / EFL_PAGE_SIZE)

/*===========================================================================*/
/* Validation helpers.                                                       */
/*===========================================================================*/

/**
 * @brief   Validate that EFL storage is properly configured.
 * @note    Call this at startup to verify linker symbols are valid.
 * @return  true if storage region is valid, false otherwise.
 */
static inline bool efl_storage_validate(void) {
    /* Check that base is before end. */
    if (EFL_STORAGE_BASE >= EFL_STORAGE_END) {
        return false;
    }
    /* Check that size is a multiple of page size. */
    if (EFL_STORAGE_SIZE % EFL_PAGE_SIZE != 0) {
        return false;
    }
    /* Check that base is page-aligned. */
    if (((size_t)EFL_STORAGE_BASE % EFL_PAGE_SIZE) != 0) {
        return false;
    }
    return true;
}

#endif /* EFL_STORAGE_H */

/** @} */
