#include "ch.h"
#include "hal.h"
#include "chprintf.h"
//#include "shell.h"

#include "portab.h"
#include "usbcfg.h"

//#define USB_DP_LINE                 PAL_LINE(GPIOA, 11U)
//#define USB_DM_LINE                 PAL_LINE(GPIOA, 12U)
//#define USB_DP_LINE_MODE            PAL_MODE_INPUT_ANALOG
//#define USB_DM_LINE_MODE            PAL_MODE_INPUT_ANALOG

int main(void) {
    halInit();
    chSysInit();
    
    /* Configure USB DP and DM Pins */
    //palSetLineMode(USB_DP_LINE, USB_DP_LINE_MODE);
    //palSetLineMode(USB_DM_LINE, USB_DM_LINE_MODE);

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

    while (true) {

        chprintf((BaseSequentialStream*)&PORTAB_SDU1, "Hello World!\r\n");

        chThdSleepMilliseconds(1500);

    }

}
