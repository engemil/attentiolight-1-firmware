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
 * @file    efl_test.h
 * @brief   EFL driver test functions header.
 * @note    These functions provide a simple way to test the EFL driver.
 *          They are compiled in but require explicit calls to execute.
 *
 * @addtogroup EFL_TEST
 * @{
 */

#ifndef EFL_TEST_H
#define EFL_TEST_H

#include "hal.h"

#if (HAL_USE_EFL == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Test result codes.                                                        */
/*===========================================================================*/

typedef enum {
    EFL_TEST_OK = 0,
    EFL_TEST_STORAGE_INVALID,
    EFL_TEST_DRIVER_START_FAILED,
    EFL_TEST_ERASE_FAILED,
    EFL_TEST_ERASE_VERIFY_FAILED,
    EFL_TEST_PROGRAM_FAILED,
    EFL_TEST_READ_FAILED,
    EFL_TEST_DATA_MISMATCH,
    EFL_TEST_DRIVER_STOP_FAILED
} efl_test_result_t;

/*===========================================================================*/
/* Function prototypes.                                                      */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Run a complete EFL driver test.
 * @details This function tests the full EFL driver functionality:
 *          1. Validates storage region configuration
 *          2. Starts the EFL driver
 *          3. Erases the first storage sector
 *          4. Verifies the erase
 *          5. Programs test data
 *          6. Reads back and verifies the data
 *          7. Stops the driver
 *
 * @param[in] stream    Optional BaseSequentialStream for debug output.
 *                      Pass NULL to disable output.
 * @return              EFL_TEST_OK if all tests pass, error code otherwise.
 *
 * @note    This test WILL ERASE DATA in the first EFL storage sector!
 *          Only call this function if you understand the consequences.
 */
efl_test_result_t efl_test_run(BaseSequentialStream *stream);

/**
 * @brief   Get a human-readable string for a test result.
 * @param[in] result    Test result code.
 * @return              String describing the result.
 */
const char *efl_test_result_str(efl_test_result_t result);

#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_EFL == TRUE */

#endif /* EFL_TEST_H */

/** @} */
