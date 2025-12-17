#include "ch.h"
#include "hal.h"
#include "chprintf.h"
//#include "shell.h"

#include "portab.h"
#include "usbcfg.h"
#include "ee_ws2812b_chibios_driver.h"
#include "ee_esp32_wifi_ble_if_driver.h"


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
     * Initializes EngEmil WS2812B Driver.
     */
    ee_ws2812b_init_driver();


    /* Configure Serial Driver SD2 (USART2) for Virtual COM Port */
    sdStart(&SD2, &serial_cfg);
    
    uint32_t test_serial_usb = 0;
    uint32_t test_serial_vcp = 0;

    while (true) {

        // TEST VCP SERIAL COMMUNICATION
        chprintf((BaseSequentialStream*)&PORTAB_SDU1, "TEST SERIAL OVER USB. Count: %U\r\n", test_serial_usb);
        
        // TEST USB SERIAL COMMUNICATION
        chprintf((BaseSequentialStream*)&SD2, "TEST SERIAL OVER STLINK VCP. Count: %U\r\n", test_serial_vcp);
        
        // TEST USER BUTTON
        if (palReadLine(LINE_USER_BUTTON) == PAL_HIGH) {
            chprintf((BaseSequentialStream*)&SD2, "BUTTON NOT PRESSED!\r\n");
        } else {
            chprintf((BaseSequentialStream*)&SD2, "BUTTON PRESSED!!!!!!!!!!!!\r\n");
        }

        test_serial_usb++;
        test_serial_vcp++;

        chThdSleepMilliseconds(500);

        ee_ws2812b_set_color_rgb_and_render(0xFF, 0x00, 0x00);
        chThdSleepMilliseconds(500);
        ee_ws2812b_set_color_rgb_and_render(0x00, 0xFF, 0x00);
        chThdSleepMilliseconds(500);
        ee_ws2812b_set_color_rgb_and_render(0x00, 0x00, 0xFF);
        chThdSleepMilliseconds(500);

    }

}
