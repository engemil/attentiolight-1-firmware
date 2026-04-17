/*
    ChibiOS - Copyright (C) 2006..2018 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

/*
   With compliance of the license:
   Portions modified from original.

   Dual CDC/ACM (IAD) configuration for AttentioLight-1.
   Based on ChibiOS USB_CDC_IAD testhal example.

   CDC0 (PORTAB_SDU1): Serial print stream (device -> host)
   CDC1 (PORTAB_SDU2): Shell command interface (bidirectional)
*/

#include "hal.h"
#include "portab.h"
#include "app_header.h"
#include "usbcfg.h"

#include <string.h>

/* Virtual serial ports over USB. */
SerialUSBDriver PORTAB_SDU1;
SerialUSBDriver PORTAB_SDU2;

/*
 * Endpoints to be used for USBD1.
 *
 * CDC0 (serial prints):  EP1 = interrupt IN, EP2 = bulk IN+OUT
 * CDC1 (shell commands): EP3 = interrupt IN, EP4 = bulk IN+OUT
 */
#define USB_INTERRUPT_REQUEST_EP_A      1
#define USB_DATA_AVAILABLE_EP_A         2
#define USB_DATA_REQUEST_EP_A           2
#define USB_INTERRUPT_REQUEST_EP_B      3
#define USB_DATA_AVAILABLE_EP_B         4
#define USB_DATA_REQUEST_EP_B           4

#define USB_INTERRUPT_REQUEST_SIZE      0x0010
#define USB_DATA_SIZE                   0x0040

/*
 * Interfaces.
 */
#define USB_NUM_INTERFACES              4
#define USB_CDC_CIF_NUM0                0
#define USB_CDC_DIF_NUM0                1
#define USB_CDC_CIF_NUM1                2
#define USB_CDC_DIF_NUM1                3

/*
 * USB Device Descriptor.
 * Uses Miscellaneous device class with IAD protocol for composite device.
 */
static const uint8_t vcom_device_descriptor_data[18] = {
  USB_DESC_DEVICE       (0x0200,        /* bcdUSB (2.0, required for IAD). */
                         0xEF,          /* bDeviceClass (Miscellaneous).    */
                         0x02,          /* bDeviceSubClass (Common Class).  */
                         0x01,          /* bDeviceProtocol (IAD).           */
                         USB_DATA_SIZE, /* bMaxPacketSize.                  */
                         USB_VID,       /* idVendor (from app_header.h).    */
                         USB_PID,       /* idProduct (from app_header.h).   */
                         0x0200,        /* bcdDevice.                       */
                         1,             /* iManufacturer.                   */
                         2,             /* iProduct.                        */
                         3,             /* iSerialNumber.                   */
                         1)             /* bNumConfigurations.              */
};

/*
 * Device Descriptor wrapper.
 */
static const USBDescriptor vcom_device_descriptor = {
  sizeof vcom_device_descriptor_data,
  vcom_device_descriptor_data
};

/*
 * CDC interface descriptor set size (without IAD).
 * Communication Interface (9) + Header FD (5) + Call Mgmt FD (5) +
 * ACM FD (4) + Union FD (5) + Interrupt EP (7) +
 * Data Interface (9) + Bulk OUT EP (7) + Bulk IN EP (7) = 58 bytes
 */
#define CDC_IF_DESC_SET_SIZE                                                \
  (USB_DESC_INTERFACE_SIZE + 5 + 5 + 4 + 5 + USB_DESC_ENDPOINT_SIZE +     \
   USB_DESC_INTERFACE_SIZE + (USB_DESC_ENDPOINT_SIZE * 2))

/*
 * CDC interface descriptor set macro.
 * Generates the complete CDC-ACM interface pair (communication + data).
 */
#define CDC_IF_DESC_SET(comIfNum, datIfNum, comInEp, datOutEp, datInEp)     \
  /* Interface Descriptor.*/                                                \
  USB_DESC_INTERFACE(                                                       \
    comIfNum,                               /* bInterfaceNumber.        */  \
    0x00,                                   /* bAlternateSetting.       */  \
    0x01,                                   /* bNumEndpoints.           */  \
    CDC_COMMUNICATION_INTERFACE_CLASS,      /* bInterfaceClass.         */  \
    CDC_ABSTRACT_CONTROL_MODEL,             /* bInterfaceSubClass.      */  \
    0x01,                                   /* bInterfaceProtocol (AT       \
                                               commands, CDC section       \
                                               4.4).                    */ \
    0),                                     /* iInterface.              */  \
  /* Header Functional Descriptor (CDC section 5.2.3).*/                    \
  USB_DESC_BYTE     (5),                    /* bLength.                 */  \
  USB_DESC_BYTE     (CDC_CS_INTERFACE),     /* bDescriptorType.         */  \
  USB_DESC_BYTE     (CDC_HEADER),           /* bDescriptorSubtype.      */  \
  USB_DESC_BCD      (0x0110),               /* bcdCDC.                  */  \
  /* Call Management Functional Descriptor.*/                               \
  USB_DESC_BYTE     (5),                    /* bFunctionLength.         */  \
  USB_DESC_BYTE     (CDC_CS_INTERFACE),     /* bDescriptorType.         */  \
  USB_DESC_BYTE     (CDC_CALL_MANAGEMENT),  /* bDescriptorSubtype.      */  \
  USB_DESC_BYTE     (0x00),                 /* bmCapabilities.          */  \
  USB_DESC_BYTE     (datIfNum),             /* bDataInterface.          */  \
  /* Abstract Control Management Functional Descriptor.*/                   \
  USB_DESC_BYTE     (4),                    /* bFunctionLength.         */  \
  USB_DESC_BYTE     (CDC_CS_INTERFACE),     /* bDescriptorType.         */  \
  USB_DESC_BYTE     (CDC_ABSTRACT_CONTROL_MANAGEMENT),                      \
  USB_DESC_BYTE     (0x02),                 /* bmCapabilities.          */  \
  /* Union Functional Descriptor.*/                                         \
  USB_DESC_BYTE     (5),                    /* bFunctionLength.         */  \
  USB_DESC_BYTE     (CDC_CS_INTERFACE),     /* bDescriptorType.         */  \
  USB_DESC_BYTE     (CDC_UNION),            /* bDescriptorSubtype.      */  \
  USB_DESC_BYTE     (comIfNum),             /* bMasterInterface.        */  \
  USB_DESC_BYTE     (datIfNum),             /* bSlaveInterface.         */  \
  /* Endpoint, Interrupt IN.*/                                              \
  USB_DESC_ENDPOINT (                                                       \
    comInEp,                                                                \
    USB_EP_MODE_TYPE_INTR,                  /* bmAttributes.            */  \
    USB_INTERRUPT_REQUEST_SIZE,             /* wMaxPacketSize.          */  \
    0xFF),                                  /* bInterval.               */  \
                                                                            \
  /* CDC Data Interface Descriptor.*/                                       \
  USB_DESC_INTERFACE(                                                       \
    datIfNum,                               /* bInterfaceNumber.        */  \
    0x00,                                   /* bAlternateSetting.       */  \
    0x02,                                   /* bNumEndpoints.           */  \
    CDC_DATA_INTERFACE_CLASS,               /* bInterfaceClass.         */  \
    0x00,                                   /* bInterfaceSubClass (CDC      \
                                               section 4.6).            */ \
    0x00,                                   /* bInterfaceProtocol (CDC      \
                                               section 4.7).            */ \
    0x00),                                  /* iInterface.              */  \
  /* Endpoint, Bulk OUT.*/                                                  \
  USB_DESC_ENDPOINT(                                                        \
    datOutEp,                               /* bEndpointAddress.        */  \
    USB_EP_MODE_TYPE_BULK,                  /* bmAttributes.            */  \
    USB_DATA_SIZE,                          /* wMaxPacketSize.          */  \
    0x00),                                  /* bInterval.               */  \
  /* Endpoint, Bulk IN.*/                                                   \
  USB_DESC_ENDPOINT(                                                        \
    datInEp,                                /* bEndpointAddress.        */  \
    USB_EP_MODE_TYPE_BULK,                  /* bmAttributes.            */  \
    USB_DATA_SIZE,                          /* wMaxPacketSize.          */  \
    0x00)                                   /* bInterval.               */

/*
 * IAD + CDC interface descriptor set size.
 */
#define IAD_CDC_IF_DESC_SET_SIZE                                            \
  (USB_DESC_INTERFACE_ASSOCIATION_SIZE + CDC_IF_DESC_SET_SIZE)

/*
 * IAD + CDC interface descriptor set macro.
 * Wraps each CDC function with an Interface Association Descriptor.
 */
#define IAD_CDC_IF_DESC_SET(comIfNum, datIfNum, comInEp, datOutEp, datInEp) \
  /* Interface Association Descriptor.*/                                    \
  USB_DESC_INTERFACE_ASSOCIATION(                                           \
    comIfNum,                               /* bFirstInterface.         */  \
    2,                                      /* bInterfaceCount.         */  \
    CDC_COMMUNICATION_INTERFACE_CLASS,      /* bFunctionClass.          */  \
    CDC_ABSTRACT_CONTROL_MODEL,             /* bFunctionSubClass.       */  \
    1,                                      /* bFunctionProtocol.       */  \
    0                                       /* iInterface.              */  \
  ),                                                                        \
  /* CDC Interface descriptor set.*/                                        \
  CDC_IF_DESC_SET(comIfNum, datIfNum, comInEp, datOutEp, datInEp)

/*
 * Configuration Descriptor tree for dual CDC/ACM with IAD.
 */
static const uint8_t vcom_configuration_descriptor_data[] = {
  /* Configuration Descriptor.*/
  USB_DESC_CONFIGURATION(
    USB_DESC_CONFIGURATION_SIZE +
    (IAD_CDC_IF_DESC_SET_SIZE * 2),         /* wTotalLength.                */
    USB_NUM_INTERFACES,                     /* bNumInterfaces.              */
    0x01,                                   /* bConfigurationValue.         */
    0,                                      /* iConfiguration.              */
    0xC0,                                   /* bmAttributes (self powered). */
    50),                                    /* bMaxPower (100mA).           */
  /* CDC0: Serial print stream (PORTAB_SDU1). */
  IAD_CDC_IF_DESC_SET(
    USB_CDC_CIF_NUM0,
    USB_CDC_DIF_NUM0,
    USB_ENDPOINT_IN(USB_INTERRUPT_REQUEST_EP_A),
    USB_ENDPOINT_OUT(USB_DATA_AVAILABLE_EP_A),
    USB_ENDPOINT_IN(USB_DATA_REQUEST_EP_A)
  ),
  /* CDC1: Shell command interface (PORTAB_SDU2). */
  IAD_CDC_IF_DESC_SET(
    USB_CDC_CIF_NUM1,
    USB_CDC_DIF_NUM1,
    USB_ENDPOINT_IN(USB_INTERRUPT_REQUEST_EP_B),
    USB_ENDPOINT_OUT(USB_DATA_AVAILABLE_EP_B),
    USB_ENDPOINT_IN(USB_DATA_REQUEST_EP_B)
  ),
};

/*
 * Configuration Descriptor wrapper.
 */
static const USBDescriptor vcom_configuration_descriptor = {
  sizeof vcom_configuration_descriptor_data,
  vcom_configuration_descriptor_data
};

/*
 * U.S. English language identifier.
 */
static const uint8_t vcom_string0[] = {
  USB_DESC_BYTE(4),                     /* bLength.                         */
  USB_DESC_BYTE(USB_DESCRIPTOR_STRING), /* bDescriptorType.                 */
  USB_DESC_WORD(0x0409)                 /* wLANGID (U.S. English).          */
};

/*
 * Vendor string.
 */
static const uint8_t vcom_string1[] = {
  USB_DESC_BYTE(22),                    /* bLength.                         */
  USB_DESC_BYTE(USB_DESCRIPTOR_STRING), /* bDescriptorType.                 */
  'E', 0, 'n', 0, 'g', 0, 'E', 0, 'm', 0, 'i', 0, 'l', 0, '.', 0,
  'i', 0, 'o', 0
};

/*
 * Device Description string.
 */
static const uint8_t vcom_string2[] = {
  USB_DESC_BYTE(32),                    /* bLength.                         */
  USB_DESC_BYTE(USB_DESCRIPTOR_STRING), /* bDescriptorType.                 */
  'A', 0, 't', 0, 't', 0, 'e', 0, 'n', 0, 't', 0, 'i', 0, 'o', 0,
  'L', 0, 'i', 0, 'g', 0, 'h', 0, 't', 0, '-', 0, '1', 0
};

/*
 * Serial Number string.
 * Populated at runtime from the STM32 96-bit unique device ID (UID).
 * Format: 24 uppercase hex characters (3 x 32-bit words).
 * Buffer size: 2 (header) + 24 * 2 (UTF-16LE) = 50 bytes.
 */
static uint8_t vcom_string3[50];

/*
 * Strings wrappers array.
 * Not const because vcom_string3 (serial number) is populated at runtime.
 */
static USBDescriptor vcom_strings[] = {
  {sizeof vcom_string0, vcom_string0},
  {sizeof vcom_string1, vcom_string1},
  {sizeof vcom_string2, vcom_string2},
  {sizeof vcom_string3, vcom_string3}
};

/*
 * STM32C0xx unique device ID register base address.
 * 96-bit UID stored as 3 x 32-bit words.
 */
#define STM32_UID_BASE  0x1FFF7550U

/**
 * @brief   Populate USB serial number string descriptor from STM32 chip UID.
 * @note    Must be called before usbStart() so the host sees the correct
 *          serial number during enumeration.
 */
void usbcfg_set_serial_from_uid(void) {
  static const char hex_chars[] = "0123456789ABCDEF";
  const uint32_t *uid = (const uint32_t *)STM32_UID_BASE;
  uint8_t hex_str[24];
  unsigned i;

  /* Format 3 x 32-bit UID words as 24 uppercase hex characters. */
  for (i = 0; i < 3; i++) {
    uint32_t w = uid[i];
    hex_str[i * 8 + 0] = hex_chars[(w >> 28) & 0xFU];
    hex_str[i * 8 + 1] = hex_chars[(w >> 24) & 0xFU];
    hex_str[i * 8 + 2] = hex_chars[(w >> 20) & 0xFU];
    hex_str[i * 8 + 3] = hex_chars[(w >> 16) & 0xFU];
    hex_str[i * 8 + 4] = hex_chars[(w >> 12) & 0xFU];
    hex_str[i * 8 + 5] = hex_chars[(w >>  8) & 0xFU];
    hex_str[i * 8 + 6] = hex_chars[(w >>  4) & 0xFU];
    hex_str[i * 8 + 7] = hex_chars[(w >>  0) & 0xFU];
  }

  /* Build USB string descriptor: header + UTF-16LE characters. */
  vcom_string3[0] = (uint8_t)sizeof(vcom_string3);   /* bLength (50)              */
  vcom_string3[1] = USB_DESCRIPTOR_STRING;            /* bDescriptorType           */
  for (i = 0; i < 24; i++) {
    vcom_string3[2 + i * 2]     = hex_str[i];         /* ASCII character           */
    vcom_string3[2 + i * 2 + 1] = 0;                  /* UTF-16LE high byte        */
  }
}

/*
 * Handles the GET_DESCRIPTOR callback. All required descriptors must be
 * handled here.
 */
static const USBDescriptor *get_descriptor(USBDriver *usbp,
                                           uint8_t dtype,
                                           uint8_t dindex,
                                           uint16_t lang) {

  (void)usbp;
  (void)lang;
  switch (dtype) {
  case USB_DESCRIPTOR_DEVICE:
    return &vcom_device_descriptor;
  case USB_DESCRIPTOR_CONFIGURATION:
    return &vcom_configuration_descriptor;
  case USB_DESCRIPTOR_STRING:
    if (dindex < 4)
      return &vcom_strings[dindex];
  }
  return NULL;
}

/*===========================================================================*/
/* CDC0 endpoint configurations (serial prints).                              */
/*===========================================================================*/

/**
 * @brief   IN EP1 state (CDC0 interrupt).
 */
static USBInEndpointState ep1instate;

/**
 * @brief   EP1 initialization structure (IN only, interrupt).
 */
static const USBEndpointConfig ep1config = {
  USB_EP_MODE_TYPE_INTR,
  NULL,
  sduInterruptTransmitted,
  NULL,
  USB_INTERRUPT_REQUEST_SIZE,
  0x0000,
  &ep1instate,
  NULL,
  1,
  NULL
};

/**
 * @brief   IN EP2 state (CDC0 bulk IN).
 */
static USBInEndpointState ep2instate;

/**
 * @brief   OUT EP2 state (CDC0 bulk OUT).
 */
static USBOutEndpointState ep2outstate;

/**
 * @brief   EP2 initialization structure (both IN and OUT, bulk).
 */
static const USBEndpointConfig ep2config = {
  USB_EP_MODE_TYPE_BULK,
  NULL,
  sduDataTransmitted,
  sduDataReceived,
  USB_DATA_SIZE,
  USB_DATA_SIZE,
  &ep2instate,
  &ep2outstate,
  2,
  NULL
};

/*===========================================================================*/
/* CDC1 endpoint configurations (shell commands).                            */
/*===========================================================================*/

/**
 * @brief   IN EP3 state (CDC1 interrupt).
 */
static USBInEndpointState ep3instate;

/**
 * @brief   EP3 initialization structure (IN only, interrupt).
 */
static const USBEndpointConfig ep3config = {
  USB_EP_MODE_TYPE_INTR,
  NULL,
  sduInterruptTransmitted,
  NULL,
  USB_INTERRUPT_REQUEST_SIZE,
  0x0000,
  &ep3instate,
  NULL,
  1,
  NULL
};

/**
 * @brief   IN EP4 state (CDC1 bulk IN).
 */
static USBInEndpointState ep4instate;

/**
 * @brief   OUT EP4 state (CDC1 bulk OUT).
 */
static USBOutEndpointState ep4outstate;

/**
 * @brief   EP4 initialization structure (both IN and OUT, bulk).
 */
static const USBEndpointConfig ep4config = {
  USB_EP_MODE_TYPE_BULK,
  NULL,
  sduDataTransmitted,
  sduDataReceived,
  USB_DATA_SIZE,
  USB_DATA_SIZE,
  &ep4instate,
  &ep4outstate,
  2,
  NULL
};

/*
 * Handles the USB driver global events.
 */
static void usb_event(USBDriver *usbp, usbevent_t event) {
  extern SerialUSBDriver PORTAB_SDU1;
  extern SerialUSBDriver PORTAB_SDU2;

  switch (event) {
  case USB_EVENT_ADDRESS:
    return;
  case USB_EVENT_CONFIGURED:
    chSysLockFromISR();

    if (usbp->state == USB_ACTIVE) {
      /* Enables the endpoints specified into the configuration.
         Note, this callback is invoked from an ISR so I-Class functions
         must be used.*/

      /* CDC0 endpoints (serial prints). */
      usbInitEndpointI(usbp, USB_INTERRUPT_REQUEST_EP_A, &ep1config);
      usbInitEndpointI(usbp, USB_DATA_REQUEST_EP_A, &ep2config);

      /* CDC1 endpoints (shell commands). */
      usbInitEndpointI(usbp, USB_INTERRUPT_REQUEST_EP_B, &ep3config);
      usbInitEndpointI(usbp, USB_DATA_REQUEST_EP_B, &ep4config);

      /* Resetting the state of both CDC subsystems. */
      sduConfigureHookI(&PORTAB_SDU1);
      sduConfigureHookI(&PORTAB_SDU2);
    }
    else if (usbp->state == USB_SELECTED) {
      usbDisableEndpointsI(usbp);
    }

    chSysUnlockFromISR();
    return;
  case USB_EVENT_RESET:
    /* Falls into.*/
  case USB_EVENT_UNCONFIGURED:
    /* Falls into.*/
  case USB_EVENT_SUSPEND:
    chSysLockFromISR();

    /* Disconnection event on suspend. */
    sduSuspendHookI(&PORTAB_SDU1);
    sduSuspendHookI(&PORTAB_SDU2);

    chSysUnlockFromISR();
    return;
  case USB_EVENT_WAKEUP:
    chSysLockFromISR();

    /* Connection event on wakeup. */
    sduWakeupHookI(&PORTAB_SDU1);
    sduWakeupHookI(&PORTAB_SDU2);

    chSysUnlockFromISR();
    return;
  case USB_EVENT_STALLED:
    return;
  }
  return;
}

/*
 * Handling messages not implemented in the default handler nor in the
 * SerialUSB handler.
 */
static bool requests_hook(USBDriver *usbp) {

  if (((usbp->setup[0] & USB_RTYPE_RECIPIENT_MASK) == USB_RTYPE_RECIPIENT_INTERFACE) &&
      (usbp->setup[1] == USB_REQ_SET_INTERFACE)) {
    usbSetupTransfer(usbp, NULL, 0, NULL);
    return true;
  }
  return sduRequestsHook(usbp);
}

/*
 * Handles the USB driver global events.
 */
static void sof_handler(USBDriver *usbp) {

  (void)usbp;

  osalSysLockFromISR();
  sduSOFHookI(&PORTAB_SDU1);
  sduSOFHookI(&PORTAB_SDU2);
  osalSysUnlockFromISR();
}

/*
 * USB driver configuration.
 */
const USBConfig usbcfg = {
  usb_event,
  get_descriptor,
  requests_hook,
  sof_handler
};

/*
 * Serial over USB driver configuration 1 (CDC0 - serial prints).
 */
const SerialUSBConfig serusbcfg1 = {
  &PORTAB_USB1,
  USB_DATA_REQUEST_EP_A,
  USB_DATA_AVAILABLE_EP_A,
  USB_INTERRUPT_REQUEST_EP_A
};

/*
 * Serial over USB driver configuration 2 (CDC1 - shell commands).
 */
const SerialUSBConfig serusbcfg2 = {
  &PORTAB_USB1,
  USB_DATA_REQUEST_EP_B,
  USB_DATA_AVAILABLE_EP_B,
  USB_INTERRUPT_REQUEST_EP_B
};
