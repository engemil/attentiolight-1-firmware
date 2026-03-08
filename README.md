# AttentioLight-1 MainBoard-1 Firmware

This is the source code (firmware) for the **AttentioLight-1 MainBoard-1** (`al1mb1`) microcontroller (STM32C071RB).

**Standalone Application Features**, with a varity of different modes.
- Solid Color Mode (shared with other modes).
- Brightness Mode (shared with other modes).
- Blinking Mode.
- Pulsation Mode.
- Effects Mode (rainbow, color cycle, breathing, candle, fire, lava lamp, Day/Night Cycle, Ocean, Northern Lights, Thunder Storm, Police, Health Pulse, and Memory).
- Traffic Light Mode (Cycle between Green, Yellow, and Red).
- Night Light Mode (3 levels of low brightness).

**Technical Features**
- RGB LED rendering for animations with a varity of use-cases.
- Real-time operative system integration with ChibiOS.
- Persistent data for run-time data storage, with EFL (Embedded Flash) Driver. Including CRC32 integrity checking and factory default restoration.
- Integration with Bootloader and application header for application firmware updates over USB.
- Low-power mode with wake-up from user button.
- Dual USB CDC/ACM interface with IAD (Interface Association Descriptors): CDC0 for debug output, CDC1 for shell commands and external control.
- ChibiOS Shell (on CDC1) with interactive commands.


## Table of Contents

- [Standalone Application](#standalone-application)
  - [Button Controls](#button-controls)
  - [LED Feedback](#led-feedback)
  - [Mode Sequence](#mode-sequence)
  - [State Sequence](#state-sequence)
  - [Mode Details](#mode-details)
- [Dependencies](#dependencies)
  - [Required Tools](#required-tools)
  - [Installation (Ubuntu/Debian)](#installation-ubuntudebian)
- [Setup Repository](#setup-repository)
- [Quick Start](#quick-start)
- [Project Structure](#project-structure)
- [Hardware](#hardware)
- [Firmware Stack](#firmware-stack)
- [Memory Map](#memory-map)
- [Libraries and Drivers](#libraries-and-drivers)
- [Debugging](#debugging)
  - [Debug Builds](#debug-builds)
  - [VS Code Debug Configurations](#vs-code-debug-configurations)
  - [Hardware Breakpoint Limitation](#hardware-breakpoint-limitation)
  - [Thread Stack Analysis](#thread-stack-analysis)
- [VSCode Tasks](#vscode-tasks)
- [Utility Scripts](#utility-scripts)
  - [EFL Memory Reader](#efl-memory-reader-workspacescriptsefl_scripts)
  - [Sign App Header](#sign-app-header-scriptssign_app_headersh)
  - [System Setup Scripts](#system-setup-scripts-scriptssystem)
  - [Memory Usage](#memory-usage)
- [Shell Commands](#shell-commands)
  - [Connecting to the Shell](#connecting-to-the-shell)
  - [ChibiOS Built-in Commands](#chibios-built-in-commands)
  - [Application Commands](#application-commands)
- [Troubleshooting](#troubleshooting)
  - [Identifying CDC Ports](#identifying-cdc-ports)
- [Bugs and Issues](#bugs-and-issues)
- [Additional Sources](#additional-sources)
- [License](#license)


## Standalone Application

The AttentioLight1 as a standalone application means it operates without any additional hardware, the device in itself only receives power from cable (USB-C). Where the only interaction is with the User Button infront of the device.


### Button Controls

| Press Duration | Action |
|----------------|--------|
| **Short** (< 1s) | Mode-specific action (cycle color, speed, etc.) |
| **Long** (2-5s) | Switch to next mode *(white flash at 2s)* |
| **Extended** (≥ 5s) | Power off |
| **Long (when OFF)** (2s) | Wake up |

### LED Feedback

- **White flash** — Long press threshold reached (2s)
- **Powerup** — Rainbow fade-in followed by blue pulse (~8s)
- **Powerdown** — Amber pulse with fading intensity (~4s)

### Mode Sequence

Modes cycle via long press (wraps after Night Light):

```
Solid Color → Brightness → Blinking → Pulsation → Effects → Traffic Light → Night Light
     ↑                                                                            │
     └────────────────────────────────────────────────────────────────────────────┘
```

### State Sequence

System states flow during power cycle:

```
BOOT → POWERUP → ACTIVE → POWERDOWN → OFF
          ↑                            │
          └────────────────────────────┘
```

### Mode Details

| Mode | Short Press Action | Options |
|------|-------------------|---------|
| **Solid Color** | Cycle through colors | 12 colors: Azure, Blue, Purple, Magenta, Pink, Red, Orange, Yellow, Lime, Green, Spring, Cyan *(shared with Blinking, Pulsation)* |
| **Brightness** | Cycle brightness level | 8 levels: 12%, 25%, 37%, 50%, 62%, 75%, 87%, 100% *(shared with all modes except Night Light)* |
| **Blinking** | Cycle blink speed | 6 speeds: Ultra Slow, Very Slow, Slow, Medium, Fast, Very Fast |
| **Pulsation** | Cycle pulse speed | 5 speeds: Ultra Slow, Very Slow, Slow, Medium, Fast |
| **Effects** | Cycle through effects | 13 effects: Rainbow, Color Cycle, Breathing, Candle, Fire, Lava Lamp, Day/Night, Ocean, Northern Lights, Thunder Storm, Police, Health Pulse, Memory |
| **Traffic Light** | Cycle traffic light colors | Green → Yellow → Red |
| **Night Light** | Cycle dim level | 4 warm amber levels (very dim) |


## Dependencies

### Required Tools
- **arm-none-eabi-gcc** (13.2.1 or later) - ARM cross-compiler.
- **make** (4.3 or later) - Build system.
- **st-flash** or **openocd** - Hardware programming.
  - NB! **stlink** v1.8.0 does not have STM32C071xx included. On Ubuntu 24.04 MUST build from source with `develop`-branch [link](https://github.com/stlink-org/stlink).
  - NB! **openocd** v0.12.0 or newer. On Ubuntu 24.04 you MUST build from source [link](https://github.com/openocd-org/openocd).
- **dfu-util** (0.11 or later) - USB Device Firmware Upgrade (DFU). Firmware uploads over USB.


### Installation (Ubuntu/Debian)
```bash
sudo apt update
sudo apt install git build-essential gcc-arm-none-eabi \
  binutils-arm-none-eabi libnewlib-arm-none-eabi \
  python3 dfu-util -y

# NB! We also need stlink-tools and openocd, but they are outdated on ubuntu 24.04.
# see the .devcontainer/Dockerfile for how to install from source, or use docker container
#sudo apt install stlink-tools openocd -y

# Debugging/Troubleshootng
RUN apt install gdb-multiarch minicom usbutils -y 
```

## Setup Repository

```bash
git clone https://github.com/engemil/ee_stm32_bootloader.git
cd ee_stm32_bootloader
git submodule update --init --recursive
```

See `ext/README.md` for detailed submodule management.


## Quick Start

**(If not already done) upload Bootloader Firmware via ST-Link (probe):**

```bash
# Update submodules for bootloader
cd bootloader && git submodule update --init --recursive
# Build bootloader (enter subfolder bootloader/bootloader)
cd bootloader && make clean && make
# Flash to device over debugger (the first stlink it sees)
st-flash erase && st-flash --reset write build/bootloader.bin 0x08000000
# Go back to project root folder
cd ../..
# Verify successful flash of bootloader to see if device has entered USB DFU mode
lsusb
```

**Upload Application Firmware via USB DFU (with Bootloader installed):**
```bash

# Upload application firmware over USB
cd fw_al1mb1 && make clean && make

# Upload test firmware (NB! This may take 5 min or more.)
sudo dfu-util -a 0 --dfuse-address 0x08004000:leave -D build/fw_al1mb1_signed.bin
```
<!-- Not working?! Might be due to tweaks for making it work with bootloader
```
# Alternatively with the ST-LINK connected
st-flash --reset write build/fw_al1mb1_signed.bin 0x08004000
```
-->

## Project Structure

```
.
├── fw_al1mb1/                      # Application firmware
│   ├── app/                        # Application source code and application drivers
│   │   ├── main.c                  # Entry point
│   │   ├── app_debug.h             # Debug macros & levels
│   │   ├── rt_config.h             # Runtime configuration
│   │   ├── app_state_machine/      # Core state machine
│   │   │   ├── animation/          # LED animation engine
│   │   │   ├── modes/              # User-selectable modes
│   │   │   │   └── effects/        # effects implementation
│   │   │   └── system_states/      # System state handlers
│   │   ├── button_driver/          # Button input with debouncing
│   │   ├── persistent_data/        # EFL settings storage
│   │   ├── shell_commands/         # USB shell command handlers
│   │   └── ws2812b_led_driver/     # WS2812B SPI protocol
│   ├── app_header/                 # Bootloader app header
│   ├── boards/                     # Board support packages
│   ├── cfg/                        # ChibiOS configuration
│   ├── libs/                       # Libraries & drivers independent/indirectly of application
│   │   ├── hal_efl_stm32c0xx/      # EFL driver for STM32C0
│   │   ├── ee_esp32_wifi_ble_if_driver/  # WiFi/BLE module interface
│   │   ├── usbcfg/                 # USB CDC configuration
│   │   └── portab/                 # Portability layer
│   ├── linker_scripts/             # Memory layout
│   ├── Makefile                    # Build file (make)
│   └── STM32C071.svd               # Debug register definitions
├── bootloader/                     # USB DFU bootloader (git submodule)
├── ext/                            # External dependencies
│   └── ChibiOS/                    # ChibiOS RTOS (git submodule)
├── scripts/                        # Utility scripts (related to system, build process, and more)
├── .devcontainer/                  # Docker dev environment
└── .vscode/                        # VS Code debug config
```


## Hardware

| Component | Details |
|-----------|---------|
| MCU | STM32C071RB (ARM Cortex-M0+, 48MHz) |
| Flash | 128KB |
| RAM | 24KB |
| LED | WS2812B addressable RGB |
| Button | User button |
| USB | USB 2.0 Full Speed, Dual CDC/ACM (IAD) |
| Debug | SWD + CDC0 debug serial, CDC1 interactive shell |
| Oscillators | 48MHz HSE, 32.768kHz LSE |
| Wireless Module| ESP32 WiFi/BLE module interface* |

(*) Code in this codebase related to the wireless module is only for interfacing with it.


## Firmware Stack

```
┌──────────────────────────────────────────────────────────────┐
│                    APPLICATION LAYER                         │
│  ┌─────────────────────────────────────────────────────────┐ │
│  │ State Machine (app_state_machine/)                      │ │
│  │   ├── System States                                     │ │
│  │   ├── Modes                                             │ │
│  │   └── Animation Engine                                  │ │
│  └─────────────────────────────────────────────────────────┘ │
│  ┌─────────────────────────────────────────────────────────┐ │
│  │ Shell / CLI (shell_commands/)                           │ │
│  │   ├── version — Firmware version query                  │ │
│  │   ├── settings — Persistent data get/set                │ │
│  │   └── dfu — Reboot into DFU bootloader                  │ │
│  └─────────────────────────────────────────────────────────┘ │
├──────────────────────────────────────────────────────────────┤
│                    APPLICATION DRIVERS                       │
│  ┌──────────────┐ ┌──────────────┐ ┌───────────────────────┐ │
│  │ WS2812B LED  │ │ Button       │ │ Persistent Data       │ │
│  │ Driver       │ │ Driver       │ │ (EFL Storage)         │ │
│  └──────────────┘ └──────────────┘ └───────────────────────┘ │
├──────────────────────────────────────────────────────────────┤
│                    APP HEADER (app_header/)                  │
│  ┌─────────────────────────────────────────────────────────┐ │
│  │ Bootloader Interface: magic, version, size, CRC32       │ │
│  │ USB Descriptors: VID/PID for DFU identification         │ │
│  └─────────────────────────────────────────────────────────┘ │
├──────────────────────────────────────────────────────────────┤
│                    LIBRARIES (libs/)                         │
│  ┌──────────────────┐ ┌──────────────────┐ ┌──────────────┐  │
│  │ hal_efl_stm32c0xx│ │ usbcfg           │ │ portab       │  │
│  │ (Flash Driver)   │ │ (Dual CDC/IAD)   │ │ (Portability)│  │
│  └──────────────────┘ └──────────────────┘ └──────────────┘  │
│  ┌──────────────────────────────────────────────────────────┐│
│  │ ee_esp32_wifi_ble_if_driver (WiFi/BLE Module Interface)  ││
│  └──────────────────────────────────────────────────────────┘│
├──────────────────────────────────────────────────────────────┤
│                    ChibiOS/RT + HAL                          │
│  ┌─────────────────────────────────────────────────────────┐ │
│  │ RT Kernel: threads, semaphores, events, mailboxes       │ │
│  │ HAL: GPIO, SPI, USB, PWR, EFL (Embedded Flash)          │ │
│  └─────────────────────────────────────────────────────────┘ │
├──────────────────────────────────────────────────────────────┤
│                    STM32C0xx LL/HAL                          │
├──────────────────────────────────────────────────────────────┤
│                    STM32C071RB Hardware                      │
└──────────────────────────────────────────────────────────────┘
```

**Key Components:**

- **ChibiOS/RT** — Real-time kernel with threads and event-driven architecture.
- **ChibiOS HAL** — Hardware abstraction for GPIO, SPI, USB, PWR, timers.
- **hal_efl_stm32c0xx** — Custom EFL driver (STM32C0 not in mainline ChibiOS).
- **USB Stack** — Dual CDC/ACM with IAD: CDC0 for debug serial, CDC1 for interactive shell. DFU mode via bootloader.
- **Shell / CLI** — ChibiOS shell on CDC1. Heap-allocated thread respawns on USB reconnection.
- **WS2812B Driver** — SPI-based protocol using DMA for timing-critical LED data.
- **Power Management** — Low-power mode with EXTI wake-up.

> **Note:** The goal is to follow this layered abstraction as closely as possible, but in practice some boundaries are crossed for performance or simplicity reasons.


## Memory Map

**STM32C071RB:** 128KB Flash, 24KB RAM

```
FLASH (128KB)                              RAM (24KB)
┌──────────────────────────────┐ 0x08020000 ┌──────────────────┐ 0x20006000
│ EFL Storage    8KB (4 pages) │ 0x0801E000 │ Heap             │
├──────────────────────────────┤            ├──────────────────┤
│                              │            │ BSS + Data       │
│ Application   ~104KB         │ 0x08004100 ├──────────────────┤
│ Code + Data                  │            │ Main Stack       │
├──────────────────────────────┤            ├──────────────────┤
│ Vectors        192B (aligned)│ 0x08004100 │ Process Stack    │
├──────────────────────────────┤            └──────────────────┘ 0x20000000
│ Padding        224B          │ 0x08004020
├──────────────────────────────┤
│ App Header     32B           │ 0x08004000
├──────────────────────────────┤
│ Bootloader     16KB          │ 0x08000000
└──────────────────────────────┘
```

| Region | Address | Size | Notes |
|--------|---------|------|-------|
| Bootloader | 0x08000000 | 16KB | Do not overwrite if you use bootloader (see bootloader submodule for more info) |
| App Header | 0x08004000 | 32B | Magic, version, size, CRC32, USB VID/PID |
| Application | 0x08004100 | ~104KB | Code, vectors, read-only data |
| EFL Storage | 0x0801E000 | 8KB | Persistent settings (4 × 2KB pages) |

## Libraries and Drivers

| Library | Description |
|---------|-------------|
| **usbcfg** | USB dual CDC/ACM configuration with IAD descriptors. Defines two virtual serial ports: CDC0 (debug prints) and CDC1 (shell commands). Manages USB device/configuration descriptors, endpoint configs, and driver lifecycle hooks. |
| **portab** | Board portability abstraction layer. Maps logical driver names (`PORTAB_SDU1`, `PORTAB_SDU2`, `PORTAB_USB1`) to ChibiOS HAL instances. |
| **hal_efl_stm32c0xx** | Custom Embedded Flash (EFL) driver for STM32C0xx. Required because STM32C0 is not yet supported in mainline ChibiOS EFL. |
| (Work-in-progress) **ee_esp32_wifi_ble_if_driver** | ESP32 WiFi/BLE module GPIO interface (placeholder). Controls enable and boot pins for ESP32-C3 WROOM module. UART communication protocol not yet implemented. |

> Note that the driver/library placement structure is a bit messy at the moment. With two approaches, one is simply having the drivers under the `app` folder, the other approach (for these) is the `libs`-folder. It is for subject to change in the future.

## Debugging

### Debug Builds

```bash
cd fw_al1mb1

make                  # Release build (no debug output)
make debug            # Debug build with all outputs
make debug LEVEL=5    # Debug build with all outputs
```

Debug levels (hierarchical):
- `0` = NONE (release)
- `1` = ERROR
- `2` = WARN  
- `3` = INFO
- `4` = DEBUG
- `5` = POWER (verbose + disables Stop mode for debugging) (default for `make debug`)




### VS Code Debug Configurations

Two debug configurations are available in `.vscode/launch.json`:

| Configuration | Description |
|---------------|-------------|
| **Rebuild and Debug (Direct)** | Bypasses bootloader — flashes app, sets VTOR/PC/SP directly |
| **Rebuild and Debug (via Bootloader)** | Flashes signed app, lets bootloader validate and jump |

Both configurations:
- Run `Rebuild Application (DEBUG)` before debugging
- Flash the signed binary (`fw_al1mb1_signed.bin`)
- Use `-Og` optimization (debug-friendly)

### Hardware Breakpoint Limitation

> **Warning:** The STM32C0 (Cortex-M0+) has only **(3-4?) hardware breakpoints**.
> Software breakpoints do not work in flash memory.
> 
> If breakpoints are not being hit, check that you haven't exceeded this limit.
> Remove unused breakpoints or use conditional breakpoints sparingly.

### Thread Stack Analysis

Debug builds (`make debug`) automatically enable thread stack analysis tools
to help detect and prevent stack overflows. All features below are **disabled
in release builds** (zero overhead).

#### 1. Runtime Stack Overflow Detection (ChibiOS)

Enabled via `CH_DBG_ENABLE_STACK_CHECK` in `cfg/chconf.h` (auto-enabled in
debug builds). ChibiOS checks each thread's stack pointer against its working
area boundary **on every context switch**. If the stack has overflowed, the
system halts immediately with `"stack overflow"`.

If you hit this halt during debugging, the faulting thread's name is available
in the debugger. Increase that thread's working area size in `app/rt_config.h`.

#### 2. Runtime Stack Watermark Reporting (Serial Output)

In debug builds (LEVEL >= 4), the main loop prints stack watermarks for all
threads every 5 seconds over USB serial port **CDC0** (debug print stream):

```
[STACK] === Thread Stack Watermarks ===
[STACK]  main              320 /  880 bytes (36%)
[STACK]  button             92 /  760 bytes (12%)
[STACK]  state_machine     408 /  960 bytes (42%)
[STACK]  animation_thread  384 /  960 bytes (40%)
[STACK] ================================
```

Each line shows `used / total bytes (percent)`. The "used" value is the
**high-water mark** (peak usage since thread creation), not current usage.

**How to read it:**
- **< 70%** — Stack size is adequate, some headroom available.
- **70-85%** — Getting tight, consider increasing the working area.
- **> 85%** — Dangerous, increase the working area size immediately.
- **100%** — Stack has overflowed (the stack check above should catch this first).

To monitor, connect to the USB serial CDC0 debug port (see [Identifying CDC Ports](#identifying-cdc-ports)):
```bash
minicom -D /dev/ttyACMx -b 115200
```

Thread working area sizes are configured in `app/rt_config.h`:
- `BTN_THREAD_WA_SIZE` — Button thread
- `APP_SM_THREAD_WA_SIZE` — State machine thread
- `APP_SM_ANIM_THREAD_WA_SIZE` — Animation thread
- `SHELL_WA_SIZE` — Shell thread (heap-allocated, respawns on USB reconnect)

> **Note:** Debug builds automatically add 512 extra bytes to each thread
> stack (via `_RT_DEBUG_EXTRA` in `rt_config.h`) to accommodate the 256-byte
> `dbg_printf_timeout()` buffer. The watermark percentages reflect the debug
> stack sizes, not the release sizes.

#### 3. Static Stack Usage Analysis (GCC `.su` files)

Debug builds compile with `-fstack-usage`, which generates `.su` (stack usage)
files showing **per-function stack frame sizes**. With LTO enabled, the output
is a single combined file:

```bash
# Show the 20 functions with largest stack usage
sort -t: -k3 -n fw_al1mb1/build/fw_al1mb1.elf.ltrans0.ltrans.su | tail -20
```

Example output:
```
./app/app_debug.h:129:20:dbg_printf_timeout    284    static
./app/main.c:54:6:init_system                   16    static
```

Each line: `file:line:col:function_name  stack_bytes  type`

This is useful for identifying which functions consume the most stack, without
needing to run the firmware. To compute worst-case stack depth for a thread,
trace the deepest call chain from the thread entry point and sum the frame sizes.


## VSCode Tasks

Build tasks available in `.vscode/tasks.json`. Run via: `Ctrl+Shift+P` → "Tasks: Run Task"

**One-Shot (Full Build & Flash)**

| Task | Description |
|------|-------------|
| One-Shot | Clean, build, and flash bootloader + application |
| One-Shot (DEBUG) | Same as above with debug builds |

**Application Tasks**

| Task | Description |
|------|-------------|
| Clean Application | Remove application build files |
| Build Application | Build application |
| Build Application (DEBUG) | Build application with debug enabled |
| Rebuild Application | Clean and build application |
| Rebuild Application (DEBUG) | Clean and build application with debug |
| Flash Application (OpenOCD) | Flash application via ST-Link SWD |
| Flash Application (st-flash) | **BROKEN** - Erases bootloader on STM32C0 before it flashes application |
| Flash Application (dfu-util) | Flash application via USB DFU |

**Bootloader Tasks**

| Task | Description |
|------|-------------|
| Clean Bootloader | Remove bootloader build files |
| Build Bootloader | Build bootloader |
| Build Bootloader (DEBUG) | Build debug bootloader |
| Rebuild Bootloader | Clean and build bootloader |
| Rebuild Bootloader (DEBUG) | Clean and build debug bootloader |
| Flash Bootloader (OpenOCD) | Flash bootloader via ST-Link SWD |
| Flash Bootloader (st-flash) | Flash bootloader via st-flash |

**Utility Tasks**

| Task | Description |
|------|-------------|
| Erase Flash (st-flash) | Erase entire flash memory |
| Serial ACM0 (minicom ttyACM0) | Open minicom on /dev/ttyACM0 |
| Serial ACM1 (minicom ttyACM1) | Open minicom on /dev/ttyACM1 |
| Monitor All Serial Comm. | Open minicom on ACM0 and ACM1 in parallel |


## Utility Scripts

### EFL Memory Reader (`./scripts/efl_scripts/`)

CLI tools for reading the EFL (Embedded Flash) storage region via ST-Link debugger.

**Quick usage:**
```bash
# Read entire 8KB EFL region
./scripts/efl_scripts/read_efl.sh

# Read specific range
./scripts/efl_scripts/read_efl.sh --offset 0 --length 65

# Save to file
./scripts/efl_scripts/read_efl.sh --output efl_backup.bin
```

**Features:**
- Hex dump with ASCII representation
- Read any offset/length within EFL region (0x0801E000 - 0x0801FFFF)
- Save to binary file
- No device halt required

See `./scripts/efl_scripts/README.md` for complete documentation.


### Sign App Header (`scripts/sign_app_header.sh`)

**NB!** Part of **BUILD!**

Signs the application binary with firmware size and CRC32 checksum for bootloader validation. Called automatically by the Makefile during build.

```bash
./scripts/sign_app_header.sh fw_al1mb1/build/fw_al1mb1.bin fw_al1mb1/build/fw_al1mb1_signed.bin
```

The script reads the raw binary, calculates the firmware size and CRC32 (from the vector table at offset 0x100 to end), and writes them into the application header at offsets 8 and 12. The signed binary is what gets uploaded via DFU.


### System Setup Scripts (`scripts/system/`)

One-time setup scripts for configuring Linux host permissions. Must be run with `sudo`.

| Script | Description |
|--------|-------------|
| `dialout_group.sh` | Adds current user to the `dialout` group for serial device access (e.g. `/dev/ttyACM0`) |
| `udevusb_stlink.sh` | Creates udev rules for ST-LINK debuggers and STM32 DFU mode, allowing non-root USB access |

```bash
sudo ./scripts/system/udevusb_stlink.sh
sudo ./scripts/system/dialout_group.sh
# Log out and back in to apply group changes
```


### Memory Usage

Check usage after building:

```bash
./scripts/check_memory_usage.sh fw_al1mb1/build/fw_al1mb1.elf
```

**NB!** The `.heap` section by default shows 100% usage of the remaining RAM by design - ChibiOS allocates all remaining RAM to the heap. Hence the dedicated script to check memory usage.


## Shell Commands

The firmware includes a ChibiOS shell on **CDC1** providing an interactive command-line interface over USB. The shell thread is dynamically allocated from the heap and respawns automatically when the USB cable is reconnected.

**NOTE**: `attentio-cli` is recommended for interacting with the device in terminal over USB (work-in-progress). Link: https://github.com/engemil/attentio-cli

### Connecting to the Shell

Connect to the CDC1 serial port (see [Identifying CDC Ports](#identifying-cdc-ports) to find the correct `/dev/ttyACMx`). In Linux you can use for example **minicom**:

```bash
minicom -D /dev/ttyACMx -b 115200
```

The shell prompt is:
```
attentio>
```

### ChibiOS Built-in Commands

ChibiOS built-in commands are provided by the ChibiOS shell module for general info, debugging, and diagnostics.  Note that they do **not** follow the `OK`/`ERROR` protocol which the application commands does.

| Command | Description |
|---------|-------------|
| `help` | Lists all available commands (including the application commands) |
| `info` | Kernel version, compiler, architecture, board name, build time |
| `echo "message"` | Echoes back the given string |
| `systime` | Prints current system tick count |
| `mem` | Heap and core memory status (fragments, free bytes) |
| `threads` | Lists all threads with stack pointers, priority, and state |
| `exit` | Terminates the shell thread (respawns automatically on next USB check) |


### Application Commands

**Application commands** follow a consistent response format:

| Result  | Format |
|---------|--------|
| Success | `[payload lines...]\r\n` followed by `OK\r\n` |
| Failure | `ERROR <message>\r\n` |

The list of **application commands**:

| Command | Description | Example |
|---------|-------------|---------|
| `version` | Returns firmware version (`<major>.<minor>.<patch>`) from the application header | `version` → `1.1.0\r\n OK\r\n` |
| `settings get <key>` | Read a persistent data field by name | `settings get serial_number` → `000000000000\r\n OK\r\n` |
| `settings set <key> <value>` | Write a persistent data field (RW fields only) | `settings set device_name MyLight` → `OK\r\n` |
| `dfu` | Reboot into DFU bootloader mode. No response — device disconnects immediately | `dfu` → *(device reboots into DFU)* |

**Notes:**
- `settings set` only works on fields with `PD_ACCESS_RW` access. Read-only fields return `ERROR key is read-only`.
- `settings get` on an unknown field returns `ERROR unknown key`.
- The `dfu` command writes magic value `0xDEADBEEF` to RAM (`0x20005FFC`) and triggers `NVIC_SystemReset()`. The host detects the USB disconnection and can proceed with DFU flashing via `dfu-util`.


## Troubleshooting

Useful CLI tools for debugging/troubleshooting: `minicom`,  `multiarch-gdb`, `usbutils` (`lsusb`)

As well as the already mentioned tools `st-flash`/`st-erase`, `dfu-util`, `openocd`

Compile for debug:
- `make debug` or `make debug LEVEL=4` - for debug info over serial communication.
- `make debug LEVEL=5` - for debug info + do not enter low-power Stop mode.


### Identifying USB Bootloader/Normal mode

Check which mode we are in with: `lsusb`
- In normal operation, expect: `Bus 001 Device 051: ID 0483:df11 EngEmil.io AttentioLight-1`
- In bootloader, expect: `Bus 001 Device 054: ID 0483:df11 EngEmil.io Bootloader DFU Mode`

### Identifying USB CDC Ports

The device exposes two USB CDC/ACM virtual serial ports. The `/dev/ttyACMx` numbers are **not stable** and depend on enumeration order. To identify which port is which:

```bash
ls -ls /dev/serial/by-id
```

| Interface Number | Role | Description |
|------------------|------|-------------|
| `IF=00` (`...-if00`) | **CDC0** | Debug print stream (read-only) |
| `IF=02` (`...-if02`) | **CDC1** | Shell command interface (bidirectional) |

Use `minicom` for serial monitoring in terminal. E.g. for `dev/ttyACM1`
```bash
minicom -D /dev/ttyACM1 -b 115200
```


## Bugs and Issues


### Bug: First time programming a STM32C071RB

(Info from https://github.com/cbiffle/stm32c0-metapac-example)

The STM32C0xx series picked up the same odd behavior from the STM32G0xx series, where the EMPTY bit in the flash controller (used to determine if there's code worth booting in the flash) seems not to get re-evaluated at reset, and only at power-on. This means the very first time you program an STM32C0xx, if you reset it, it will bounce right back into the ROM like it's not programmed.

To fix this, power cycle it. This is only necessary when starting with a factory-fresh part.


## Additional Sources

- ChibiOS 21.11.4 Full documentation: https://chibiforge.org/doc/21.11/full_rm/
- ChibiOS repository: https://github.com/ChibiOS/ChibiOS


## License

MIT License, see `LICENSE`-file for details.

Portions of this project incorporates code from:
- **ChibiOS**, Apache License 2.0.
- **STMicroelectronics**, Apache License 2.0.
- **ee_stm32_bootloader**, MIT License.

For submodule lincenses, see individual repository LICENSE files for details.
