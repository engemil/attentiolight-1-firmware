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

#include "ch.h"
#include "hal.h"
#include "chprintf.h"
//#include "shell.h"

#include "portab.h"
#include "usbcfg.h"
#include "ws2812b_led_driver.h"
#include "ee_esp32_wifi_ble_if_driver.h"
#include "button_driver.h"


/* Serial Configuration for Virtual COM Port */
static SerialConfig serial_cfg = {
    .speed  = 115200,           // Baud rate
    .cr1    = 0,                // No parity, 8-bit data (default)
    .cr2    = 0,                // No specific control settings
    .cr3    = 0                 // No hardware flow control
};

/*
 * Button event callback function.
 * Called from button thread context (safe to use RTOS functions).
 */
static void on_button_event(button_event_t event) {
    switch (event) {
    case BTN_EVT_SHORT_PRESS:
        chprintf((BaseSequentialStream*)&PORTAB_SDU1, "Button: SHORT PRESS\r\n");
        break;
    case BTN_EVT_LONG_PRESS_START:
        chprintf((BaseSequentialStream*)&PORTAB_SDU1, "Button: LONG PRESS started...\r\n");
        break;
    case BTN_EVT_LONG_PRESS_RELEASE:
        chprintf((BaseSequentialStream*)&PORTAB_SDU1, "Button: LONG PRESS released\r\n");
        break;
    case BTN_EVT_LONGEST_PRESS_START:
        chprintf((BaseSequentialStream*)&PORTAB_SDU1, "Button: LONGEST PRESS started!\r\n");
        break;
    case BTN_EVT_LONGEST_PRESS_RELEASE:
        chprintf((BaseSequentialStream*)&PORTAB_SDU1, "Button: LONGEST PRESS released\r\n");
        break;
    default:
        break;
    }
}

int main(void) {
    halInit();
    chSysInit();

    /*
     * Board-dependent initialization.
     */
    portab_setup();

    /*
     * Initializes a serial-over-USB CDC driver.
     */
    sduObjectInit(&PORTAB_SDU1);
    sduStart(&PORTAB_SDU1, &serusbcfg);

    /*
     * Activates the USB driver and then the USB bus pull-up on D+.
     * Note, a delay is inserted in order to not have to disconnect the cable
     * after a reset.
     */
    usbDisconnectBus(serusbcfg.usbp);
    chThdSleepMilliseconds(1500);
    usbStart(serusbcfg.usbp, &usbcfg);
    usbConnectBus(serusbcfg.usbp);

    /*
     * Initializes WS2812B LED Driver.
     */
    ws2812b_led_driver_init();

    /*
     * Initializes EngEmil ESP32 Wifi Bluetooth Interface Driver.
     */
    //init_ee_esp32_wifi_ble_if_driver();
    //disable_ee_esp32_wifi_ble_if_driver();
    set_program_mode_ee_esp32_wifi_ble_if_driver();

    /* Configure Serial Driver SD2 (USART2) for Virtual COM Port */
    sdStart(&SD2, &serial_cfg);

    /*
     * Initializes Button Driver.
     * Uses interrupt-driven detection with a dedicated thread.
     */
    button_init(LINE_USER_BUTTON, PAL_LOW);
    button_register_callback(on_button_event);
    button_start();
    
    uint32_t test_serial_usb = 0;
    uint32_t test_serial_vcp = 0;

    while (true) {

        // TEST VCP SERIAL COMMUNICATION
        //chprintf((BaseSequentialStream*)&PORTAB_SDU1, "TEST SERIAL OVER USB. Count: %U\r\n", test_serial_usb);
        
        // TEST USB SERIAL COMMUNICATION
        chprintf((BaseSequentialStream*)&SD2, "TEST SERIAL OVER STLINK VCP. Count: %U\r\n", test_serial_vcp);

        test_serial_usb++;
        test_serial_vcp++;

        chThdSleepMilliseconds(500);

        // NB! The first bit is the LSB, not the MSB, hence 0x80 is the same as 1 for the LED.
        // Add a function to handle MSB/LSB first in the ws2812b_led_driver, and add option to initialize with a bool for MSB/LSBfirst and a default of LSBfirst.
        ws2812b_led_driver_set_color_rgb_and_render(0x80, 0x00, 0x00);
        chThdSleepMilliseconds(500);
        ws2812b_led_driver_set_color_rgb_and_render(0x00, 0x80, 0x00);
        chThdSleepMilliseconds(500);
        ws2812b_led_driver_set_color_rgb_and_render(0x00, 0x00, 0x80);
        chThdSleepMilliseconds(500);

    }

}
