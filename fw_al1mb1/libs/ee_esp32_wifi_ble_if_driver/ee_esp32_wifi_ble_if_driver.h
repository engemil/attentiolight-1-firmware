/*
MIT License

Copyright (c) 2025 EngEmil

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
 * @file ee_esp32_wifi_ble_if_driver.h
 * 
 * @brief EngEmil ESP32 Wifi Bluetooth Interface Driver
 * 
 */

#ifndef _EE_ESP32_WIFI_BLE_IF_DRIVER_
#define _EE_ESP32_WIFI_BLE_IF_DRIVER_

#include <stdint.h>
#include <string.h>

#include "hal.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Initialize EngEmil ESP32 Wifi Bluetooth Interface Driver.
 * 
 * @return uint8_t status code, 0 success, nonzero on error
 */
uint8_t init_ee_esp32_wifi_ble_if_driver(void);

/**
 * @brief Start EngEmil ESP32 Wifi Bluetooth Interface Driver.
 * 
 * @return uint8_t status code, 0 success, nonzero on error
 */
uint8_t start_ee_esp32_wifi_ble_if_driver(void);


#ifdef __cplusplus
}
#endif

#endif /* _EE_ESP32_WIFI_BLE_IF_DRIVER_ */