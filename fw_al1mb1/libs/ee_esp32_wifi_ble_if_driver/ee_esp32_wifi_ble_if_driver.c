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
 * @file ee_esp32_wifi_ble_if_driver.c
 * 
 * @brief EngEmil ESP32 Wifi Bluetooth Interface Driver.
 * 
 */

#include "ee_esp32_wifi_ble_if_driver.h"


uint8_t init_ee_esp32_wifi_ble_if_driver(void){
    // Initialization code here

    enable_ee_esp32_wifi_ble_if_driver();
    
    return 0;
}

uint8_t enable_ee_esp32_wifi_ble_if_driver(void){    
    palSetLine(LINE_WBM_EN); // Set pin back to normal. It does not disable the ESP32.
    return 0;
}

uint8_t disable_ee_esp32_wifi_ble_if_driver(void){    
    palClearLine(LINE_WBM_EN); // Set pin back to normal. It disables the ESP32.
    return 0;
}

uint8_t set_program_mode_ee_esp32_wifi_ble_if_driver(void){
    palClearLine(LINE_WBM_EN); /// Reset/"turn off" the ESP32.
    chThdSleepMilliseconds(500);

    palClearLine(LINE_WBM_BOOT_OPT); // to GND, for bootloader mode.
    palSetLine(LINE_WBM_EN); /// Turn on ESP32 in bootloader mode

    chThdSleepMilliseconds(1000);
    
    palSetLine(LINE_WBM_BOOT_OPT); // Release from GND (not need to hold it low after boot).

    return 0;
}
