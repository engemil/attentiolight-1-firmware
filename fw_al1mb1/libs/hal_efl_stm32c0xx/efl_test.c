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
 * @file    efl_test.c
 * @brief   EFL driver test functions implementation.
 *
 * @addtogroup EFL_TEST
 * @{
 */

#include "hal.h"

#if (HAL_USE_EFL == TRUE) || defined(__DOXYGEN__)

#include "efl_test.h"
#include "efl_storage.h"
#include "chprintf.h"
#include <string.h>

/*===========================================================================*/
/* Local definitions.                                                        */
/*===========================================================================*/

/* Test pattern size - must be less than page size (2048 bytes). */
#define EFL_TEST_DATA_SIZE      64

/*===========================================================================*/
/* Local variables.                                                          */
/*===========================================================================*/

/* Test data pattern. */
static const uint8_t efl_test_pattern[EFL_TEST_DATA_SIZE] = {
    0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE,
    0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0,
    0xA5, 0x5A, 0xF0, 0x0F, 0x55, 0xAA, 0x33, 0xCC,
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80,
    0xFF, 0xFE, 0xFD, 0xFC, 0xFB, 0xFA, 0xF9, 0xF8,
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
    0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
};

/* Read buffer. */
static uint8_t efl_test_read_buffer[EFL_TEST_DATA_SIZE];

/*===========================================================================*/
/* Local functions.                                                          */
/*===========================================================================*/

/**
 * @brief   Print a message if stream is available.
 */
#define TEST_PRINT(stream, fmt, ...) \
    do { \
        if ((stream) != NULL) { \
            chprintf((stream), fmt, ##__VA_ARGS__); \
        } \
    } while(0)

/*===========================================================================*/
/* Exported functions.                                                       */
/*===========================================================================*/

const char *efl_test_result_str(efl_test_result_t result) {
    switch (result) {
    case EFL_TEST_OK:
        return "OK";
    case EFL_TEST_STORAGE_INVALID:
        return "Storage configuration invalid";
    case EFL_TEST_DRIVER_START_FAILED:
        return "Driver start failed";
    case EFL_TEST_ERASE_FAILED:
        return "Erase failed";
    case EFL_TEST_ERASE_VERIFY_FAILED:
        return "Erase verify failed";
    case EFL_TEST_PROGRAM_FAILED:
        return "Program failed";
    case EFL_TEST_READ_FAILED:
        return "Read failed";
    case EFL_TEST_DATA_MISMATCH:
        return "Data mismatch";
    case EFL_TEST_DRIVER_STOP_FAILED:
        return "Driver stop failed";
    default:
        return "Unknown error";
    }
}

efl_test_result_t efl_test_run(BaseSequentialStream *stream) {
    flash_error_t ferr;
    flash_sector_t sector;
    flash_offset_t offset;
    uint32_t wait_ms;

    TEST_PRINT(stream, "\r\n=== EFL Driver Test ===\r\n");

    /* Step 1: Validate storage configuration. */
    TEST_PRINT(stream, "1. Validating storage configuration...\r\n");
    if (!efl_storage_validate()) {
        TEST_PRINT(stream, "   FAILED: Storage configuration invalid\r\n");
        return EFL_TEST_STORAGE_INVALID;
    }
    TEST_PRINT(stream, "   Storage base: 0x%08X\r\n", (uint32_t)EFL_STORAGE_BASE);
    TEST_PRINT(stream, "   Storage size: %u bytes (%u pages)\r\n",
               (uint32_t)EFL_STORAGE_SIZE, (uint32_t)EFL_STORAGE_PAGES);
    TEST_PRINT(stream, "   First sector: %u\r\n", (uint32_t)EFL_STORAGE_FIRST_SECTOR);
    TEST_PRINT(stream, "   OK\r\n");

    /* Step 2: Start EFL driver. */
    TEST_PRINT(stream, "2. Starting EFL driver...\r\n");
    eflStart(&EFLD1, NULL);
    TEST_PRINT(stream, "   OK\r\n");

    /* Step 3: Erase first storage sector. */
    sector = EFL_STORAGE_FIRST_SECTOR;
    TEST_PRINT(stream, "3. Erasing sector %u...\r\n", sector);
    ferr = flashStartEraseSector(&EFLD1, sector);
    if (ferr != FLASH_NO_ERROR) {
        TEST_PRINT(stream, "   FAILED: Start erase returned %d\r\n", ferr);
        eflStop(&EFLD1);
        return EFL_TEST_ERASE_FAILED;
    }

    /* Wait for erase to complete. */
    do {
        chThdSleepMilliseconds(STM32_FLASH_WAIT_TIME_MS);
        ferr = flashQueryErase(&EFLD1, &wait_ms);
    } while (ferr == FLASH_BUSY_ERASING);

    if (ferr != FLASH_NO_ERROR) {
        TEST_PRINT(stream, "   FAILED: Query erase returned %d\r\n", ferr);
        eflStop(&EFLD1);
        return EFL_TEST_ERASE_FAILED;
    }
    TEST_PRINT(stream, "   OK\r\n");

    /* Step 4: Verify erase. */
    TEST_PRINT(stream, "4. Verifying erase...\r\n");
    ferr = flashVerifyErase(&EFLD1, sector);
    if (ferr != FLASH_NO_ERROR) {
        TEST_PRINT(stream, "   FAILED: Verify erase returned %d\r\n", ferr);
        eflStop(&EFLD1);
        return EFL_TEST_ERASE_VERIFY_FAILED;
    }
    TEST_PRINT(stream, "   OK\r\n");

    /* Step 5: Program test data. */
    /* Calculate offset from flash base to our storage sector. */
    offset = (flash_offset_t)(EFL_STORAGE_BASE - (uint8_t *)FLASH_BASE);
    TEST_PRINT(stream, "5. Programming %u bytes at offset 0x%08X...\r\n",
               EFL_TEST_DATA_SIZE, offset);
    ferr = flashProgram(&EFLD1, offset, EFL_TEST_DATA_SIZE, efl_test_pattern);
    if (ferr != FLASH_NO_ERROR) {
        TEST_PRINT(stream, "   FAILED: Program returned %d\r\n", ferr);
        eflStop(&EFLD1);
        return EFL_TEST_PROGRAM_FAILED;
    }
    TEST_PRINT(stream, "   OK\r\n");

    /* Step 6: Read back and verify. */
    TEST_PRINT(stream, "6. Reading back and verifying...\r\n");
    memset(efl_test_read_buffer, 0, sizeof(efl_test_read_buffer));
    ferr = flashRead(&EFLD1, offset, EFL_TEST_DATA_SIZE, efl_test_read_buffer);
    if (ferr != FLASH_NO_ERROR) {
        TEST_PRINT(stream, "   FAILED: Read returned %d\r\n", ferr);
        eflStop(&EFLD1);
        return EFL_TEST_READ_FAILED;
    }

    if (memcmp(efl_test_pattern, efl_test_read_buffer, EFL_TEST_DATA_SIZE) != 0) {
        TEST_PRINT(stream, "   FAILED: Data mismatch!\r\n");
        TEST_PRINT(stream, "   Expected: ");
        for (int i = 0; i < 8; i++) {
            TEST_PRINT(stream, "%02X ", efl_test_pattern[i]);
        }
        TEST_PRINT(stream, "...\r\n");
        TEST_PRINT(stream, "   Got:      ");
        for (int i = 0; i < 8; i++) {
            TEST_PRINT(stream, "%02X ", efl_test_read_buffer[i]);
        }
        TEST_PRINT(stream, "...\r\n");
        eflStop(&EFLD1);
        return EFL_TEST_DATA_MISMATCH;
    }
    TEST_PRINT(stream, "   OK\r\n");

    /* Step 7: Stop driver. */
    TEST_PRINT(stream, "7. Stopping EFL driver...\r\n");
    eflStop(&EFLD1);
    TEST_PRINT(stream, "   OK\r\n");

    TEST_PRINT(stream, "\r\n=== All EFL Tests PASSED ===\r\n\r\n");
    return EFL_TEST_OK;
}

#endif /* HAL_USE_EFL == TRUE */

/** @} */
