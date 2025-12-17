# EngEmil ESP32 Wifi Bluetooth Interface Driver

EngEmil ESP32 Wifi Bluetooth Interface Driver is developed for the ESP32 C3 WROOM and is written in C with C++ wrap.

NOTES Controlling ESP32 C3 WROOM
- Enable pin for ESP32 is; output, otype pushpull, low speed, pullup, ODR High (disabled)
    - To enable ESP32, call palClearPad(), wait x ms, then call palSetPad()
- Boot option pin for ESP32 is; output, otype opendrain, low speed, floating, ODR High (normal boot)
    - To enter bootloader mode, call palClearPad() before enabling ESP32
    - After programming, set pin back to palSetPad()
