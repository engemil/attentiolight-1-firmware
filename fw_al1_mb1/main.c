#include "ch.h"
#include "hal.h"

#define VIRTUAL_COM_TX_LINE         LINE_VCP_TX
#define VIRTUAL_COM_RX_LINE         LINE_VCP_RX
#define VIRTUAL_COM_TX_LINE_MODE    PAL_MODE_ALTERNATE(7) | \
                                    PAL_STM32_OSPEED_MID2 | \
                                    PAL_STM32_PUPDR_FLOATING | \
                                    PAL_STM32_OTYPE_PUSHPULL
#define VIRTUAL_COM_RX_LINE_MODE    PAL_MODE_ALTERNATE(3) | \
                                    PAL_STM32_OSPEED_MID2 | \
                                    PAL_STM32_PUPDR_FLOATING | \
                                    PAL_STM32_OTYPE_PUSHPULL

// USB configuration structure for SerialUSBDriver
static const SerialUSBConfig serialusbcfg = {
  &USBD1,               // USB driver instance
  USB_DATA_SIZE,        // Bulk OUT endpoint size (default 64 bytes)
  USB_DATA_SIZE,        // Bulk IN endpoint size
  USB_INT_SIZE,         // Interrupt IN endpoint size (default 8 bytes)
  NULL                  // No callback for USB events (optional)
};

// USB device descriptor
static const uint8_t vcom_device_descriptor_data[18] = {
  USB_DESC_DEVICE(0x0110,  // bcdUSB (1.1)
                  0x02,    // bDeviceClass (CDC)
                  0x00,    // bDeviceSubClass
                  0x00,    // bDeviceProtocol
                  0x40,    // bMaxPacketSize (64 bytes)
                  0x0483,  // idVendor (STMicroelectronics)
                  0x5740,  // idProduct
                  0x0200,  // bcdDevice
                  1,       // iManufacturer
                  2,       // iProduct
                  3,       // iSerialNumber
                  1)       // bNumConfigurations
};

// USB configuration descriptor
static const uint8_t vcom_configuration_descriptor_data[] = {
  // Configuration descriptor
  USB_DESC_CONFIGURATION(67,  // wTotalLength
                         0x02, // bNumInterfaces
                         0x01, // bConfigurationValue
                         0,    // iConfiguration
                         0xC0, // bmAttributes (self-powered)
                         50),  // bMaxPower (100mA)
  // Interface descriptor (CDC control)
  USB_DESC_INTERFACE(0x00,  // bInterfaceNumber
                     0x00,  // bAlternateSetting
                     0x01,  // bNumEndpoints
                     0x02,  // bInterfaceClass (CDC)
                     0x02,  // bInterfaceSubClass (ACM)
                     0x01,  // bInterfaceProtocol
                     0),    // iInterface
  // Functional descriptors for CDC
  USB_DESC_CDC_HEADER(0x0110),           // bcdCDC
  USB_DESC_CDC_CALL_MGMT(0x00, 0x00),    // bDataInterface
  USB_DESC_CDC_ACM(0x02),                // bmCapabilities
  USB_DESC_CDC_UNION(0x00, 0x01),        // bSlaveInterface0
  // Endpoint descriptor (Interrupt IN)
  USB_DESC_ENDPOINT(USB_INT_EP | 0x80,   // bEndpointAddress (IN)
                    0x03,                // bmAttributes (Interrupt)
                    USB_INT_SIZE,        // wMaxPacketSize
                    0xFF),               // bInterval
  // Interface descriptor (CDC data)
  USB_DESC_INTERFACE(0x01,  // bInterfaceNumber
                     0x00,  // bAlternateSetting
                     0x02,  // bNumEndpoints
                     0x0A,  // bInterfaceClass (CDC Data)
                     0x00,  // bInterfaceSubClass
                     0x00,  // bInterfaceProtocol
                     0),    // iInterface
  // Endpoint descriptor (Bulk OUT)
  USB_DESC_ENDPOINT(USB_DATA_EP,         // bEndpointAddress (OUT)
                    0x02,                // bmAttributes (Bulk)
                    USB_DATA_SIZE,       // wMaxPacketSize
                    0x00),               // bInterval
  // Endpoint descriptor (Bulk IN)
  USB_DESC_ENDPOINT(USB_DATA_EP | 0x80,  // bEndpointAddress (IN)
                    0x02,               // bmAttributes (Bulk)
                    USB_DATA_SIZE,      // wMaxPacketSize
                    0x00)               // bInterval
};

// USB string descriptors
static const uint8_t vcom_strings[] = {
  USB_DESC_STR(0x0409),                     // Language ID (English)
  USB_DESC_STR('S', 0, 'T', 0, 'M', 0),     // Manufacturer
  USB_DESC_STR('S', 0, 'T', 0, 'M', 0, '3', 0, '2', 0, ' ', 0, 'V', 0, 'C', 0, 'O', 0, 'M', 0), // Product
  USB_DESC_STR('0', 0, '0', 0, '0', 0, '1', 0) // Serial Number
};

static const USBDescriptor vcom_strings_descriptor = {
  sizeof vcom_strings,
  vcom_strings
};

// USB descriptor callbacks
static const USBDescriptor *get_descriptor(USBDriver *usbp, uint8_t dtype, uint8_t dindex, uint16_t lang) {
  (void)usbp;
  (void)lang;
  switch (dtype) {
    case USB_DESCRIPTOR_DEVICE:
      return &vcom_device_descriptor;
    case USB_DESCRIPTOR_CONFIGURATION:
      return &vcom_configuration_descriptor;
    case USB_DESCRIPTOR_STRING:
      return &vcom_strings_descriptor;
  }
  return NULL;
}

// USB configuration callback
static void usb_event(USBDriver *usbp, usbevent_t event) {
  switch (event) {
    case USB_EVENT_CONFIGURED:
      chSysLockFromISR();
      sduConfigureHookI(&SDU1);
      chSysUnlockFromISR();
      break;
    case USB_EVENT_SUSPEND:
      chSysLockFromISR();
      sduSuspendHookI(&SDU1);
      chSysUnlockFromISR();
      break;
    default:
      break;
  }
}

// USB driver configuration
static const USBConfig usbcfg = {
  usb_event,                // Event callback
  get_descriptor,          // Descriptor callback
  NULL,                    // Requests hook (NULL for default)
  NULL                     // SOF handler (NULL if unused)
};

int main(void) {
    halInit();
    chSysInit();

    /* ChibiOS/HAL and ChibiOS/RT initialization. */
    halInit();
    chSysInit();
    
    /* Configuring USART2 (Virtual COM Port) related GPIO. 
        This configuration is already performed at board level
        initialization but is here reported only as example
        of how to configure GPIO at application level. */
    palSetPadMode(GPIOA, GPIOA_USART2_TX, PAL_MODE_ALTERNATE(7) | 
        PAL_STM32_OSPEED_MID2 | 
        PAL_STM32_PUPDR_FLOATING |
        PAL_STM32_OTYPE_PUSHPULL);
    palSetPadMode(GPIOA, GPIOA_USART2_RX, PAL_MODE_ALTERNATE(7) | 
        PAL_STM32_OSPEED_MID2 | 
        PAL_STM32_PUPDR_FLOATING | 
        PAL_STM32_OTYPE_PUSHPULL);
    /*
    palSetLineMode(LINE_USART2_TX, PAL_MODE_ALTERNATE(7) | 
        PAL_STM32_OSPEED_MID2 | 
        PAL_STM32_PUPDR_FLOATING |
        PAL_STM32_OTYPE_PUSHPULL);
    palSetLineMode(LINE_USART2_RX, PAL_MODE_ALTERNATE(7) | 
        PAL_STM32_OSPEED_MID2 | 
        PAL_STM32_PUPDR_FLOATING |
        PAL_STM32_OTYPE_PUSHPULL);
    */

    /* Starting Serial Driver with our configuration. */
    sduObjectInit(&SDU1);
    sduStart(&SDU1, &serialusbcfg);

    // Activate USB driver
    usbDisconnectBus(&USBD1);
    //chThdSleepMilliseconds(1000); // Wait for USB re-enumeration
    usbStart(&USBD1, &usbcfg);
    usbConnectBus(&USBD1);


    while (true) {

        /* Printing a string using Serial USB Driver.*/
        //sdWrite((BaseSequentialStream*)&SDU1, (uint8_t*)"Hello World!\r\n", 14U);


        if (SDU1.config->usbp->state == USB_ACTIVE) {
            // Echo received characters
            msg_t c = sdGetTimeout(&SDU1, TIME_IMMEDIATE);
            if (c != MSG_TIMEOUT) {
                sdPut(&SDU1, (uint8_t)c);
            }
            // Periodically send "Hello World"
            chprintf((BaseSequentialStream*)&SDU1, "Hello World\r\n");
        }

    }
}
