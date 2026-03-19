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
 * @file    shellconf.h
 * @brief   ChibiOS Shell configuration overrides.
 *
 * @details This file is included by the ChibiOS shell module when
 *          SHELL_CONFIG_FILE is defined. It overrides default shell
 *          settings for the AttentioLight-1 application.
 */

#ifndef SHELLCONF_H
#define SHELLCONF_H

/*===========================================================================*/
/* Shell Settings                                                            */
/*===========================================================================*/

/**
 * @brief   Shell prompt string.
 */
#define SHELL_PROMPT_STR            "attentio> "

/**
 * @brief   Disable built-in test command.
 * @details Saves significant flash by not pulling in the ChibiOS test
 *          framework.
 */
#define SHELL_CMD_TEST_ENABLED      FALSE

/**
 * @brief   Disable built-in info command.
 * @details We provide our own "metadata" command that includes all the
 *          ChibiOS system info plus device-specific metadata.
 */
#define SHELL_CMD_INFO_ENABLED      FALSE

#endif /* SHELLCONF_H */
