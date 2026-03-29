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
*/

#ifndef USBCFG_H
#define USBCFG_H

extern const USBConfig usbcfg;
extern const SerialUSBConfig serusbcfg1;
extern const SerialUSBConfig serusbcfg2;
extern SerialUSBDriver PORTAB_SDU1;
extern SerialUSBDriver PORTAB_SDU2;

/**
 * @brief   Populate USB serial number descriptor from STM32 chip UID.
 * @note    Must be called before usbStart() so the host sees the correct
 *          serial number during enumeration.
 */
void usbcfg_set_serial_from_uid(void);

#endif  /* USBCFG_H */
