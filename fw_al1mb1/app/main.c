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

#include "portab.h"
#include "usbcfg.h"
#include "ws2812b_led_driver.h"
//#include "ee_esp32_wifi_ble_if_driver.h"
#include "button_driver.h"
#include "animation_thread.h"
#include "app_state_machine.h"
#include "app_log.h"
#include "micb.h"
#include "usb_adapter.h"

/* Persistent Data Storage (EFL (Embedded Flash)) */
#include "device_metadata.h"
#include "persistent_data.h"

/* Real-time configuration (thread priorities, stack sizes) */
#include "rt_config.h"


/*===========================================================================*/
/* Button event routing                                                      */
/*===========================================================================*/

/**
 * @brief   Map button_driver event to AP button event type.
 */
static ap_button_event_t map_button_event(button_event_t event) {
    switch (event) {
    case BTN_EVT_SHORT_PRESS:           return AP_BTN_EVT_SHORT_PRESS;
    case BTN_EVT_LONG_PRESS_START:      return AP_BTN_EVT_LONG_PRESS_START;
    case BTN_EVT_LONG_PRESS_RELEASE:    return AP_BTN_EVT_LONG_PRESS_RELEASE;
    case BTN_EVT_EXTENDED_PRESS_START:  return AP_BTN_EVT_EXTENDED_PRESS_START;
    case BTN_EVT_EXTENDED_PRESS_RELEASE:return AP_BTN_EVT_EXTENDED_PRESS_RELEASE;
    default:                            return AP_BTN_EVT_SHORT_PRESS;
    }
}

/**
 * @brief   Cached reference to the state machine button callback.
 * @details Set during init, before the wrapper callback is registered.
 */
static button_callback_t sm_button_callback = NULL;

/**
 * @brief   Button event wrapper callback.
 * @details Routes button events to:
 *          1. The state machine (always — handles power off, mode changes).
 *          2. The MICB (when in REMOTE mode — forwards as AP event to host).
 *
 * @param[in] event     Button event from the button driver.
 */
static void button_event_wrapper(button_event_t event) {
    /* Always route to state machine for standalone behavior. */
    if (sm_button_callback != NULL) {
        sm_button_callback(event);
    }

    /* Additionally forward to MICB if in REMOTE mode. */
    if (micb_get_mode() == MICB_MODE_REMOTE) {
        micb_forward_button_event(map_button_event(event));
    }
}


/* Serial Configuration for Virtual COM Port */
static SerialConfig serial_cfg = {
    .speed  = 115200,           // Baud rate
    .cr1    = 0,                // No parity, 8-bit data (default)
    .cr2    = 0,                // No specific control settings
    .cr3    = 0                 // No hardware flow control
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
     * Board-dependent initialization.
     */
    portab_setup();

    /*
     * Initializes dual serial-over-USB CDC drivers.
     * CDC0 (SDU1): Serial log output stream
     * CDC1 (SDU2): Attentio Protocol (AP) interface
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
    usbcfg_set_serial_from_uid();
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

    LOG_INFO("MAIN initializing application...");

    /*
     * Initialize device metadata storage (EFL page 0).
     * Loads production-programmed metadata from flash.
     */
    LOG_DEBUG("MAIN device_metadata_init()...");
    md_result_t md_result = device_metadata_init();
    if (md_result != MD_OK) {
        LOG_WARN("MAIN device_metadata_init() returned: %s (using defaults)",
                 device_metadata_result_str(md_result));
    } else {
        LOG_DEBUG("MAIN device_metadata_init() OK");
    }

    /*
     * Initialize persistent settings storage (EFL page 1).
     * Loads user-configurable settings from flash (or defaults on first boot).
     */
    LOG_DEBUG("MAIN persistent_data_init()...");
    pd_result_t pd_result = persistent_data_init();
    if (pd_result != PD_OK) {
        LOG_WARN("MAIN persistent_data_init() returned: %s (using defaults)",
                 persistent_data_result_str(pd_result));
    } else {
        LOG_DEBUG("MAIN persistent_data_init() OK");
    }

    /*
     * Initialize logging system.
     * Loads boot log level from persistent storage.
     */
    log_init();

    const pd_data_t *pd = persistent_data_get();
    if (pd != NULL) {
        LOG_DEBUG("Device name: %s", pd->device_name);
        LOG_DEBUG("Log level: %d", pd->log_level);
    }

    /*
     * Initialize MICB (Multi-Interface Control Broker).
     * Must be after persistent_data and log_init so settings are available.
     */
    LOG_DEBUG("MAIN micb_init()...");
    micb_init();
    LOG_DEBUG("MAIN micb_init() OK");

    /*
     * Initialize USB adapter for AP protocol on CDC1.
     * Registers send callback with MICB.
     */
    LOG_DEBUG("MAIN usb_adapter_init()...");
    usb_adapter_init();
    LOG_DEBUG("MAIN usb_adapter_init() OK");

    /*
     * Initialize button driver.
     * Creates button thread in STOPPED state (started later by state machine).
     */
    LOG_DEBUG("MAIN button_init()...");
    if (button_init(LINE_USER_BUTTON, PAL_LOW) != 0) {
        LOG_ERROR("MAIN button_init() FAILED");
    }
    LOG_DEBUG("MAIN button_init() OK");

    /*
     * Initialize animation subsystem.
     * Creates animation thread and initializes LED driver.
     */
    LOG_DEBUG("MAIN anim_thread_init()...");
    if (anim_thread_init() != 0) {
        LOG_ERROR("MAIN anim_thread_init() FAILED");
    }
    LOG_DEBUG("MAIN anim_thread_init() OK");

    LOG_DEBUG("MAIN anim_thread_start()...");
    if (anim_thread_start() != 0) {
        LOG_ERROR("MAIN anim_thread_start() FAILED");
    }
    LOG_DEBUG("MAIN anim_thread_start() OK");

    /*
     * Initialize Application State Machine.
     */
    LOG_DEBUG("MAIN app_sm_init()...");
    if (app_sm_init() != 0) {
        LOG_ERROR("MAIN app_sm_init() FAILED");
    }
    LOG_DEBUG("MAIN app_sm_init() OK");

    /*
     * Register button callback with wrapper that routes to both
     * the state machine and MICB (when in REMOTE mode).
     */
    LOG_DEBUG("MAIN registering button callback...");
    sm_button_callback = app_sm_get_button_event_callback();
    button_register_callback(button_event_wrapper);
    LOG_DEBUG("MAIN button callback registered");

    /*
     * Start Application State Machine.
     */
    LOG_DEBUG("MAIN app_sm_start()...");
    if (app_sm_start() != 0) {
        LOG_ERROR("MAIN app_sm_start() FAILED");
    }
    LOG_DEBUG("MAIN state machine started");

    /*
     * Start USB adapter thread (reads AP packets from CDC1).
     */
    LOG_DEBUG("MAIN usb_adapter_start()...");
    usb_adapter_start();
    LOG_DEBUG("MAIN usb_adapter_start() OK");

    LOG_INFO("Initialized AttentioLight-1 by EngEmil.io");
}

int main(void) {
    
    /* Initialize system (HAL, board, USB, etc.) */
    init_system();

    /* Add a delay to ensure USB (Serial) is fully initialized before starting the (main) application part */
    chThdSleepMilliseconds(500);

    /* Initialize application threads and subsystems. */
    init_application_systems();

    /*
     * Main thread: idle loop.
     *
     * Application work is handled by dedicated threads:
     * - Button driver thread (input)
     * - State machine thread (standalone logic)
     * - Animation thread (LED rendering)
     * - USB adapter thread (AP protocol handling)
     */
    while (true) {

#if (APP_STACK_WATERMARK == 1)
        /* Periodically print thread stack watermarks (peak usage).
         * Helps right-size working areas defined in rt_config.h.
         * Controlled by APP_STACK_WATERMARK (defaults to 1 in debug builds).
         * See also: scripts/analyse/memory_report.sh for static analysis. */
        log_print_stack_usage();
#endif

#if (APP_HEAP_ANALYSIS == 1)
        /* Periodically print ChibiOS heap status (free memory, fragmentation).
         * Controlled by APP_HEAP_ANALYSIS (defaults to 1 in debug builds). */
        log_print_heap_status();
#endif

        chThdSleepMilliseconds(1000);
    }

}
