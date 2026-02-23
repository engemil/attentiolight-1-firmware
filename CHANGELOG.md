# Changelog

All notable changes to the EngEmil STM32 Bootloader project will be documented in this file.

**Version Format:** MAJOR.MINOR.PATCH
- **MAJOR:** Incompatible API/protocol changes
- **MINOR:** New features (backward compatible)
- **PATCH:** Bug fixes (backward compatible)

[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

Note: Update `app_header.h` when publishing new version.

---

## [Development] (2026-02-23)

Changed
- Minor tweak in udev stlink script.

---

## [1.0.0] (2026-02-22)

NB! Notes added here during development to keep track of changes.

Added
- Added USB PID and VID for app header file, for bootloader.
- **Persistent Data** organizing/structuring the persistentt data, interfacing with EFL driver.
- **EFL (Embedded Flash) Driver** to store persistent data on Flash.
- Additional debug level (level 5) for disabling low-power Stop mode when debugging system.
- Lower-power mode **Stop mode** implemented, with wake-up from User Button (EXTI) (and in future with USB).
- WS2812B Driver **DMA Error Counter**.
- **rt_config.h** to gather all real-time related configurations together.
- Brightness config setting for **change mode feedback**.
- **global/shared color** and brightness between solid, brightness, blinking and pulsation states.
- New **effects** in *effect mode**; breathing, candle, fire, lava lamp, day/night cycle, ocean, northern lights, thunder storm, police, health pulse, memory.
- New **powerdown animation sequence**.
- New **powerup animation sequence**.
- New **Render modes**; static, transitions, and continuous, to handle different types of modes and animations.
- **Improved RGB LED rendering** by making it deterministic with a configurable rendering rate.
- **Debug** prints to serial communication (over USB) with levels.
- Script to check Memory Useage (in scripts folder).
- New threads: **Animation Thread** and **State Machine Threads**.
- **Modes** (available in the **Active** state): solid, brightness, blink, pulse, traffic light, night light, effects, and external control.
- **Operational Modes**, aka. different ways the RGB LED light is used.
- **Application State Machine** with boot, powerup, active, shutdown (powerdown), and off states.
- **License note** added/adjusted on most source files.
- **Button Driver** for reading the button presses and "decode" different type of presses, as well as callback functionality.
- **LED Test Thread** for testing LED rendering in it's own thread.
- **Board Files** for ST_NUCLEO64_C071RB added (from ChibiOS) in boards-folder.
- **ChibiOS** as submodule
- **Integrated bootloader requirements** to be able to upload application firmware over USB.
- **Bootloader** added as submodule.
- Application Firmware first iteration as a prototype. With test code, LED light driver, USB driver, and VCP.
- Application built on ChibiOS.

Fixed
- Fixed problem with early initialization debug prints to show up on serial com (USB).
- Corrected bad state call flow, transition from **boot** state to **powerup** state.
- **Most Significant Bit (MSB)** order for WS2812B LED driver corrected.

Changed
- Moved linker scripts to a subfolder.
- Simplified include paths, since Makefile make all available.
- Separated and moved the configurations for each subsystem, state or mode.
- Moved effects for effect mode to their separate files.
- Moved thread and application sub-/system initialization to main.c. Simpler overview of real-time related initializations.
- Tweaked levels of a handful of debug prints.
- Changed going from off state to startup state, requires only reach the long button press to start (hold in until classified as long press).
- Moved powerup and powerdown state's timers from the state machine file and into their state file. 
- Renamed **Strength** mode to **Brightness** mode.
- Restructured the use of **button driver**, moved code away from main.c and to app_state_machine.c, and initializing button driver from state_boot.c/state_boot_enter-function.
- Renamed **startup** state to **powerup**, and **shutdown** state to **powerdown**.
- Renamed **Animation mode** to **Effects mode**.
- Changed **Button Driver** start and stop calls, to avoid affecting the the start-up/power-up and shutdown/power-down animation.
- Renamed **Longest button press** to **extended button press**.
- Renamed **ee_ws2812b_chibios_driver** to **ws2812b_led_driver** and move it from **libs** to **app** folder.
- Updated **.devcontainer files** with better container structure.
- Removed **ChibiOS** command from .vscode files. 

Removed
- **strobe effect** removed.
- Redundant **powerup timer**.
- Unecessary skip powerup button press.

---


