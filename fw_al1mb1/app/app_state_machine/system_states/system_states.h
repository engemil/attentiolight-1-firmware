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
 * @file    system_states.h
 * @brief   System state handlers interface.
 */

#ifndef SYSTEM_STATES_H
#define SYSTEM_STATES_H

#include "../app_state_machine.h"

/*===========================================================================*/
/* System State Handlers                                                     */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Boot state - enter handler.
 */
void state_boot_enter(void);

/**
 * @brief   Boot state - process input.
 */
void state_boot_process(app_sm_input_t input);

/**
 * @brief   Boot state - exit handler.
 */
void state_boot_exit(void);

/**
 * @brief   Startup state - enter handler.
 */
void state_startup_enter(void);

/**
 * @brief   Startup state - process input.
 */
void state_startup_process(app_sm_input_t input);

/**
 * @brief   Startup state - exit handler.
 */
void state_startup_exit(void);

/**
 * @brief   Active state - enter handler.
 */
void state_active_enter(void);

/**
 * @brief   Active state - process input.
 */
void state_active_process(app_sm_input_t input);

/**
 * @brief   Active state - exit handler.
 */
void state_active_exit(void);

/**
 * @brief   Shutdown state - enter handler.
 */
void state_shutdown_enter(void);

/**
 * @brief   Shutdown state - process input.
 */
void state_shutdown_process(app_sm_input_t input);

/**
 * @brief   Shutdown state - exit handler.
 */
void state_shutdown_exit(void);

/**
 * @brief   Off state - enter handler.
 */
void state_off_enter(void);

/**
 * @brief   Off state - process input.
 */
void state_off_process(app_sm_input_t input);

/**
 * @brief   Off state - exit handler.
 */
void state_off_exit(void);

#ifdef __cplusplus
}
#endif

#endif /* SYSTEM_STATES_H */
