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
 * @file    mode_traffic_light_config.h
 * @brief   Configuration defines for the Traffic Light mode.
 */

#ifndef MODE_TRAFFIC_LIGHT_CONFIG_H
#define MODE_TRAFFIC_LIGHT_CONFIG_H

/*===========================================================================*/
/* Traffic Light Animation Configuration                                     */
/*===========================================================================*/

/**
 * @brief   Traffic light red duration in milliseconds.
 */
#ifndef APP_SM_TRAFFIC_RED_MS
#define APP_SM_TRAFFIC_RED_MS           3000
#endif

/**
 * @brief   Traffic light yellow duration in milliseconds.
 */
#ifndef APP_SM_TRAFFIC_YELLOW_MS
#define APP_SM_TRAFFIC_YELLOW_MS        1000
#endif

/**
 * @brief   Traffic light green duration in milliseconds.
 */
#ifndef APP_SM_TRAFFIC_GREEN_MS
#define APP_SM_TRAFFIC_GREEN_MS         3000
#endif

#endif /* MODE_TRAFFIC_LIGHT_CONFIG_H */
