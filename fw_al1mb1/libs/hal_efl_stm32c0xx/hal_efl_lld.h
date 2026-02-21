/*
    ChibiOS - Copyright (C) 2006-2026 Giovanni Di Sirio.

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

/**
 * @file    hal_efl_lld.h
 * @brief   STM32C0xx Embedded Flash subsystem low level driver header.
 * @note    Ported from STM32G0xx driver for STM32C0xx series support.
 *          Supports STM32C011, C031, C051, C071, C091, C092 variants.
 *
 * @addtogroup HAL_EFL
 * @{
 */

#ifndef HAL_EFL_LLD_H
#define HAL_EFL_LLD_H

#if (HAL_USE_EFL == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @brief   STM32C0xx has only single bank flash.
 */
#define STM32_FLASH_NUMBER_OF_BANKS         1

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    STM32C0xx configuration options
 * @{
 */
/**
 * @brief   Suggested wait time during erase operations polling.
 */
#if !defined(STM32_FLASH_WAIT_TIME_MS) || defined(__DOXYGEN__)
#define STM32_FLASH_WAIT_TIME_MS            5
#endif
/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if defined(STM32C011xx) || defined(STM32C031xx) ||                         \
    defined(STM32C051xx) || defined(STM32C071xx) ||                         \
    defined(STM32C091xx) || defined(STM32C092xx) ||                         \
    defined(__DOXYGEN__)

/* Flash size register - STM32C0xx specific (different from G0xx). */
#define STM32_FLASH_SIZE_REGISTER           0x1FFF75A0
#define STM32_FLASH_SIZE_SCALE              1024U

/*
 * STM32C0xx Flash organization:
 * - C011: 32KB flash, 16 x 2KB pages
 * - C031: 32KB flash, 16 x 2KB pages
 * - C051: 32KB flash, 16 x 2KB pages
 * - C071: 64KB or 128KB flash, 32 or 64 x 2KB pages
 * - C091: 256KB flash, 128 x 2KB pages
 * - C092: 256KB flash, 128 x 2KB pages
 *
 * All variants use 2KB page size.
 */
#define STM32_FLASH_PAGE_SIZE               2048U

/* Flash size constants. */
#define STM32_FLASH_SIZE_32K                32U
#define STM32_FLASH_SIZE_64K                64U
#define STM32_FLASH_SIZE_128K               128U
#define STM32_FLASH_SIZE_256K               256U

/* Sector counts for each size. */
#define STM32_FLASH_SECTORS_TOTAL_32K       16
#define STM32_FLASH_SECTORS_TOTAL_64K       32
#define STM32_FLASH_SECTORS_TOTAL_128K      64
#define STM32_FLASH_SECTORS_TOTAL_256K      128

/* Sector size calculation (same for all - 2KB). */
#define STM32_FLASH_SECTOR_SIZE             STM32_FLASH_PAGE_SIZE

#else
#error "This EFL driver does not support the selected device"
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/* A flash size declaration. */
typedef struct {
  const flash_descriptor_t* desc;
} efl_lld_size_t;

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/**
 * @brief   Low level fields of the embedded flash driver structure.
 */
#define efl_lld_driver_fields                                               \
  /* Flash registers.*/                                                     \
  FLASH_TypeDef             *flash;                                         \
  const flash_descriptor_t  *descriptor;

/**
 * @brief   Low level fields of the embedded flash configuration structure.
 */
#define efl_lld_config_fields                                               \
  /* Dummy configuration, it is not needed.*/                               \
  uint32_t                  dummy;

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if !defined(__DOXYGEN__)
extern EFlashDriver EFLD1;
#endif

#ifdef __cplusplus
extern "C" {
#endif
  void efl_lld_init(void);
  void efl_lld_start(EFlashDriver *eflp);
  void efl_lld_stop(EFlashDriver *eflp);
  const flash_descriptor_t *efl_lld_get_descriptor(void *instance);
  flash_error_t efl_lld_read(void *instance, flash_offset_t offset,
                             size_t n, uint8_t *rp);
  flash_error_t efl_lld_program(void *instance, flash_offset_t offset,
                                size_t n, const uint8_t *pp);
  flash_error_t efl_lld_start_erase_all(void *instance);
  flash_error_t efl_lld_start_erase_sector(void *instance,
                                           flash_sector_t sector);
  flash_error_t efl_lld_query_erase(void *instance, uint32_t *msec);
  flash_error_t efl_lld_verify_erase(void *instance, flash_sector_t sector);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_EFL == TRUE */

#endif /* HAL_EFL_LLD_H */

/** @} */
