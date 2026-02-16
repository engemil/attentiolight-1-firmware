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
/* #include "led_test.h" */  /* Replaced by app_state_machine */
#include "app_state_machine.h"
#include "app_debug.h"


/* Serial Configuration for Virtual COM Port */
static SerialConfig serial_cfg = {
    .speed  = 115200,           // Baud rate
    .cr1    = 0,                // No parity, 8-bit data (default)
    .cr2    = 0,                // No specific control settings
    .cr3    = 0                 // No hardware flow control
};

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
     * Initializes EngEmil ESP32 Wifi Bluetooth Interface Driver.
     */
    //init_ee_esp32_wifi_ble_if_driver();
    //disable_ee_esp32_wifi_ble_if_driver();
    //set_program_mode_ee_esp32_wifi_ble_if_driver();

    /* Configure Serial Driver SD2 (USART2) for Virtual COM Port */
    sdStart(&SD2, &serial_cfg);

    /*
     * NOTE: WS2812B LED Driver is now initialized by the app_state_machine
     * via anim_thread_start() -> ws2812b_led_driver_start().
     * The driver has protection against double-initialization.
     */
    //ws2812b_led_driver_init();
    
    /*
     * NOTE: Button Driver is now initialized by the app_state_machine
     * in state_boot_enter() via app_sm_init_button().
     * button_start() is called later in state_active_enter().
     */

    // Add a delay to ensure ChibiOS and USB (Serial) is fully initialized before starting the (main) application part.
    chThdSleepMilliseconds(200);

    /*
     * Initializes and starts Application State Machine.
     */
    DBG_DEBUG("MAIN app_sm_init()...");
    app_sm_init();
    DBG_DEBUG("MAIN app_sm_start()...");
    app_sm_start();
    DBG_INFO("MAIN state machine started");
    
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

    }

}
