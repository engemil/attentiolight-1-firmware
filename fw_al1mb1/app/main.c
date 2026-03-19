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
#include "shell.h"

#include "portab.h"
#include "usbcfg.h"
#include "ws2812b_led_driver.h"
//#include "ee_esp32_wifi_ble_if_driver.h"
#include "button_driver.h"
#include "animation_thread.h"
#include "app_state_machine.h"
#include "app_debug.h"

/* Persistent Data Storage (EFL (Embedded Flash)) */
#include "device_metadata.h"
#include "persistent_data.h"

/* Real-time configuration (thread priorities, stack sizes) */
#include "rt_config.h"

/* Shell Commands */
#include "cmd_version.h"
#include "cmd_metadata.h"
#include "cmd_settings.h"
#include "cmd_dfu.h"


/* Serial Configuration for Virtual COM Port */
static SerialConfig serial_cfg = {
    .speed  = 115200,           // Baud rate
    .cr1    = 0,                // No parity, 8-bit data (default)
    .cr2    = 0,                // No specific control settings
    .cr3    = 0                 // No hardware flow control
};

/*
 * Shell command table.
 * Commands registered here are dispatched by the ChibiOS shell module
 * when received on CDC1 (PORTAB_SDU2).
 */
static const ShellCommand shell_commands[] = {
    {"version",  cmd_version},
    {"metadata", cmd_metadata},
    {"settings", cmd_settings},
    {"dfu",      cmd_dfu},
    {NULL, NULL}
};

/*
 * Shell configuration.
 * Binds the shell to CDC1 (PORTAB_SDU2) and registers our command table.
 */
static const ShellConfig shell_cfg = {
    .sc_channel  = (BaseSequentialStream *)&PORTAB_SDU2,
    .sc_commands = shell_commands
};

/*
 * System initialization.
*/
void init_system(void) {
    /*
     * ChibiOS HAL and RTOS initialization.
     */
    halInit();
    chSysInit();

    /*
     * Initialize shell module (event source for shell termination).
     */
    shellInit();

    /*
     * Board-dependent initialization.
     */
    portab_setup();

    /*
     * Initializes dual serial-over-USB CDC drivers.
     * CDC0 (SDU1): Debug print stream
     * CDC1 (SDU2): Shell command interface
     */
    sduObjectInit(&PORTAB_SDU1);
    sduStart(&PORTAB_SDU1, &serusbcfg1);
    sduObjectInit(&PORTAB_SDU2);
    sduStart(&PORTAB_SDU2, &serusbcfg2);

    /*
     * Activates the USB driver and then the USB bus pull-up on D+.
     * Note, a delay is inserted in order to not have to disconnect the cable
     * after a reset.
     */
    usbDisconnectBus(serusbcfg1.usbp);
    chThdSleepMilliseconds(1500);
    usbStart(serusbcfg1.usbp, &usbcfg);
    usbConnectBus(serusbcfg1.usbp);

    /*
     * Initializes EngEmil ESP32 Wifi Bluetooth Interface Driver.
     */
    /* TO DO: Enable ESP32 driver and integrate with external control mode. */
    /* TO DO: Implement UART communication protocol with ESP32-C3. */
    //init_ee_esp32_wifi_ble_if_driver();
    //disable_ee_esp32_wifi_ble_if_driver();
    //set_program_mode_ee_esp32_wifi_ble_if_driver();

    /* Configure Serial Driver SD2 (USART2) for Virtual COM Port */
    sdStart(&SD2, &serial_cfg);
}

/*
 * Initializes application threads and subsystems.
*/
void init_application_systems(void){

    DBG_INFO("MAIN initializing application...");

    /*
     * Initialize device metadata storage (EFL page 0).
     * Loads production-programmed metadata (serial number) from flash.
     */
    DBG_DEBUG("MAIN device_metadata_init()...");
    md_result_t md_result = device_metadata_init();
    if (md_result != MD_OK) {
        DBG_WARN("MAIN device_metadata_init() returned: %s (using defaults)",
                 device_metadata_result_str(md_result));
    } else {
        DBG_DEBUG("MAIN device_metadata_init() OK");
        const md_data_t *md = device_metadata_get();
        if (md != NULL) {
            DBG_DEBUG("Serial: %s", md->serial_number);
        }
    }

    /*
     * Initialize persistent settings storage (EFL page 1).
     * Loads user-configurable settings from flash (or defaults on first boot).
     */
    DBG_DEBUG("MAIN persistent_data_init()...");
    pd_result_t pd_result = persistent_data_init();
    if (pd_result != PD_OK) {
        DBG_WARN("MAIN persistent_data_init() returned: %s (using defaults)",
                 persistent_data_result_str(pd_result));
    } else {
        DBG_DEBUG("MAIN persistent_data_init() OK");
        const pd_data_t *pd = persistent_data_get();
        if (pd != NULL) {
            DBG_DEBUG("Device name: %s", pd->device_name);
        }
    }

    /*
     * Initialize button driver.
     * Creates button thread in STOPPED state (started later by state machine).
     */
    DBG_DEBUG("MAIN button_init()...");
    if (button_init(LINE_USER_BUTTON, PAL_LOW) != 0) {
        DBG_ERROR("MAIN button_init() FAILED");
    }
    DBG_DEBUG("MAIN button_init() OK");

    /*
     * Initialize animation subsystem.
     * Creates animation thread and initializes LED driver.
     */
    DBG_DEBUG("MAIN anim_thread_init()...");
    if (anim_thread_init() != 0) {
        DBG_ERROR("MAIN anim_thread_init() FAILED");
    }
    DBG_DEBUG("MAIN anim_thread_init() OK");

    DBG_DEBUG("MAIN anim_thread_start()...");
    if (anim_thread_start() != 0) {
        DBG_ERROR("MAIN anim_thread_start() FAILED");
    }
    DBG_DEBUG("MAIN anim_thread_start() OK");

    /*
     * Initialize Application State Machine.
     */
    DBG_DEBUG("MAIN app_sm_init()...");
    if (app_sm_init() != 0) {
        DBG_ERROR("MAIN app_sm_init() FAILED");
    }
    DBG_DEBUG("MAIN app_sm_init() OK");

    /*
     * Register button callback with state machine.
     */
    DBG_DEBUG("MAIN registering button callback...");
    button_register_callback(app_sm_get_button_event_callback());
    DBG_DEBUG("MAIN button callback registered");

    /*
     * Start Application State Machine.
     */
    DBG_DEBUG("MAIN app_sm_start()...");
    if (app_sm_start() != 0) {
        DBG_ERROR("MAIN app_sm_start() FAILED");
    }
    DBG_DEBUG("MAIN state machine started");

    DBG_INFO("Initialized AttentioLight-1 by EngEmil.io");
}

int main(void) {
    
    /* Initialize system (HAL, board, USB, etc.) */
    init_system();

    /* Add a delay to ensure USB (Serial) is fully initialized before starting the (main) application part */
    chThdSleepMilliseconds(500);

    /* Initialize application threads and subsystems. */
    init_application_systems();

    /*
     * Main thread: shell respawn loop.
     *
     * The ChibiOS shell thread runs on CDC1 (PORTAB_SDU2). When the USB
     * host connects, the shell thread is created from the heap. When the
     * shell exits (USB disconnect, 'exit' command, or error), the thread
     * terminates and is re-spawned after a brief delay.
     *
     * This pattern handles USB cable reconnection gracefully — each new
     * connection gets a fresh shell instance.
     */
    while (true) {

#if (DBG_ENABLE_STACK_WATERMARK == 1)
        /* Periodically print stack watermarks for all threads.
         * This helps right-size thread working areas by showing peak usage.
         * Enable by setting DBG_ENABLE_STACK_WATERMARK to 1 in app_debug.h */
        dbg_print_stack_usage();
#endif

        /* Only spawn shell when USB CDC is active (host connected). */
        if (PORTAB_SDU2.config->usbp->state == USB_ACTIVE) {
            thread_t *shelltp = chThdCreateFromHeap(NULL, SHELL_WA_SIZE,
                                                    "shell",
                                                    RT_SHELL_THREAD_PRIORITY,
                                                    shellThread,
                                                    (void *)&shell_cfg);
            if (shelltp != NULL) {
                chThdWait(shelltp);
            }
        }

        chThdSleepMilliseconds(1000);
    }

}
