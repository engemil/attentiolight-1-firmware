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
- Dual USB CDC/ACM interface with IAD (Interface Association Descriptors): CDC0 for debug output, CDC1 for Attentio Protocol (AP) interface and external control.
- Attentio Protocol (AP) on CDC1 with binary packet framing for session-based device control.


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
- [Persistent Storage (EFL)](#persistent-storage-efl)
- [Libraries and Drivers](#libraries-and-drivers)
- [Debugging](#debugging)
  - [Debug Builds](#debug-builds)
  - [VS Code Debug Configurations](#vs-code-debug-configurations)
  - [Hardware Breakpoint Limitation](#hardware-breakpoint-limitation)
  - [Runtime Memory Analysis](#runtime-memory-analysis)
- [VSCode Tasks](#vscode-tasks)
- [Utility Scripts](#utility-scripts)
  - [EFL Memory Reader](#efl-memory-reader-workspacescriptsefl_scripts)
  - [Sign App Header](#sign-app-header-scriptsbuildsign_app_headersh)
  - [System Setup Scripts](#system-setup-scripts-scriptssystem)
  - [Memory Usage](#memory-usage)
- [Attentio Protocol (AP)](#attentio-protocol-ap)
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
git clone https://github.com/engemil/attentiolight-1-firmware.git
cd attentiolight-1-firmware
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
│   │   ├── app_log.c/h             # Runtime logging system (LOG_* macros)
│   │   ├── rt_config.h             # Runtime configuration (priorities, stack sizes)
│   │   ├── debug_config.h          # Debug diagnostic flags (watermark, heap analysis)
│   │   ├── app_state_machine/      # Core state machine
│   │   │   ├── animation/          # LED animation engine
│   │   │   └── standalone/         # Standalone mode code
│   │   │       ├── standalone_state.c/h  # Standalone globals (brightness, color, mode)
│   │   │       ├── standalone_config.h   # Standalone tuning constants
│   │   │       ├── modes/          # User-selectable modes
│   │   │       │   └── effects/    # Effects implementation
│   │   │       └── system_states/  # System state handlers
│   │   ├── attentio_protocol/      # AP packet format and definitions
│   │   ├── button_driver/          # Button input with debouncing
│   │   ├── micb/                   # Multi-Interface Control Broker
│   │   ├── persistent_data/        # EFL settings storage
│   │   ├── usb_adapter/            # AP packet parser on CDC1
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
| Debug | SWD + CDC0 debug serial, CDC1 Attentio Protocol (AP) |
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
│  │ Attentio Protocol / MICB (micb/, attentio_protocol/)  │ │
│  │   ├── Session management (claim/release/takeover)     │ │
│  │   ├── LED control (RGB, HSV, brightness, effects)     │ │
│  │   ├── Power management (on/off)                       │ │
│  │   ├── Device queries (state, caps, metadata)          │ │
│  │   ├── Settings (list/get/set) and log level control   │ │
│  │   └── DFU enter                                       │ │
│  └─────────────────────────────────────────────────────────┘ │
├──────────────────────────────────────────────────────────────┤
│                    APPLICATION DRIVERS                       │
│  ┌──────────────┐ ┌──────────────┐ ┌───────────────────────┐ │
│  │ WS2812B LED  │ │ Button       │ │ Persistent Data (EFL) │ │
│  │ Driver       │ │ Driver       │ │ (Metadata + Settings) │ │
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
- **USB Stack** — Dual CDC/ACM with IAD: CDC0 for debug serial, CDC1 for Attentio Protocol interface. DFU mode via bootloader.
- **Attentio Protocol / MICB** — Binary protocol on CDC1 with session-based access control. USB adapter thread parses AP packets and routes commands to the Multi-Interface Control Broker.
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
| EFL Storage | 0x0801E000 | 8KB | Persistent data (4 × 2KB pages) |


## Persistent Storage (EFL)

The firmware uses Embedded Flash (EFL) to store persistent data that survives power cycles. The 8KB EFL region is divided into two separate storage areas with different purposes:

### Storage Architecture

```
EFL Region (8KB = 4 pages × 2KB)
┌─────────────────────────────────┐ 0x0801E000
│ Page 0: Device Metadata (2KB)   │  ← Production-programmed, read-only
│   - Header (magic, version)     │
│   - _reserved                   │  ← (serial_number removed, now from chip UID)
│   - CRC32                       │
├─────────────────────────────────┤ 0x0801E800
│ Page 1: User Settings (2KB)     │  ← User-configurable, read-write
│   - Header (magic, version)     │
│   - device_name                 │
│   - log_level                   │  ← Default log level (persisted across reboots)
│   - CRC32                       │
├─────────────────────────────────┤ 0x0801F000
│ Page 2-3: Reserved (4KB)        │  ← Available for future use
└─────────────────────────────────┘ 0x08020000
```

> **Note:** The `GET_METADATA` AP command aggregates data from multiple sources:
> - `device_name` — User-assigned name from persistent settings
> - `serial_number` — STM32 hardware UID register (alias for `chip_uid`)
> - `chip_uid` — STM32 hardware UID register (read-only silicon, 24 hex chars)
> - `firmware_version` — Application header struct
> - `device_model`, `hardware_revision`, `board` — Board BSP (`board.h`)
> - `build_date`, `build_time`, `compiler` — Compile-time defines
> - `chibios_kernel`, `architecture`, `core_variant`, `platform`, `chibios_port_info` — ChibiOS
> - `protocol_version` — AP protocol version string
> - `uptime` — Device uptime in seconds

### Metadata vs Settings

| Aspect | Metadata (Page 0) | Settings (Page 1) |
|--------|-------------------|-------------------|
| **Purpose** | Production-programmed device identity | User-configurable preferences |
| **Access** | Read-only (via AP protocol) | Read-write (via AP protocol) |
| **AP Command** | `GET_METADATA` | `SETTINGS_LIST` / `SETTINGS_GET` / `SETTINGS_SET` |
| **Factory Reset** | NOT affected | Reset to defaults |
| **Programming** | During production/provisioning | During device operation |
| **Magic** | `0x4D444154` ("MDAT") | `0x50444154` ("PDAT") |


## Libraries and Drivers

| Library | Description |
|---------|-------------|
| **usbcfg** | USB dual CDC/ACM configuration with IAD descriptors. Defines two virtual serial ports: CDC0 (serial prints) and CDC1 (Attentio Protocol interface). Manages USB device/configuration descriptors, endpoint configs, and driver lifecycle hooks. |
| **portab** | Board portability abstraction layer. Maps logical driver names (`PORTAB_SDU1`, `PORTAB_SDU2`, `PORTAB_USB1`) to ChibiOS HAL instances. |
| **hal_efl_stm32c0xx** | Custom Embedded Flash (EFL) driver for STM32C0xx. Required because STM32C0 is not yet supported in mainline ChibiOS EFL. |
| (Work-in-progress) **ee_esp32_wifi_ble_if_driver** | ESP32 WiFi/BLE module GPIO interface (placeholder). Controls enable and boot pins for ESP32-C3 WROOM module. UART communication protocol not yet implemented. |

> Note that the driver/library placement structure is a bit messy at the moment. With two approaches, one is simply having the drivers under the `app` folder, the other approach (for these) is the `libs`-folder. It is for subject to change in the future.

## Debugging

### Debug Builds

```bash
cd fw_al1mb1

make                  # Release build (-O2, Stop mode enabled)
make debug            # Debug build (-Og, Stop mode disabled, stack checks enabled)
```

Log levels are now **runtime-configurable** (all log code is always compiled in):
- `0` = NONE — no output
- `1` = ERROR — critical errors only
- `2` = WARN — errors + warnings
- `3` = INFO — + state/mode changes (default boot level)
- `4` = DEBUG — everything (verbose)

The boot default is loaded from persistent storage (`default_loglevel` setting).
Override at runtime via the `LOG_SET_LEVEL` AP command (no reboot required).

`make debug` adds `-DAPP_DEBUG_BUILD=1` which enables ChibiOS stack overflow
checks, stack fill patterns for watermark analysis, and disables Stop mode to
keep the debugger connected. It also enables diagnostic output (stack
watermarks, heap status) controlled by flags in `app/debug_config.h`.




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

### Runtime Memory Analysis

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

In debug builds, the main loop prints thread stack watermarks every ~1 second
over USB serial port **CDC0** (serial print stream):

```
[STACK] --- Thread Stack Watermarks ---
[STACK]  main              320 / 1024 bytes (31%)
[STACK]  idle              104 /  152 bytes (68%)
[STACK]  button            196 /  984 bytes (19%)
[STACK]  state_machine     248 / 1240 bytes (20%)
[STACK]  animation_thread  344 / 1240 bytes (27%)
[STACK]  usb_ap            656 /  984 bytes (66%)
[STACK] -----------------------------------
```

Controlled by the `APP_STACK_WATERMARK` compile-time flag (defined in
`app/debug_config.h`). Defaults to `1` in debug builds, `0` otherwise. To
suppress watermark output even in a debug build:

```bash
make debug UDEFS="-DAPP_STACK_WATERMARK=0"
```

This flag is **independent of the runtime log level** — setting `LOG_LEVEL_NONE`
does not suppress watermark output; only `APP_STACK_WATERMARK=0` does.

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
- `USB_ADAPTER_WA_SIZE` — USB adapter / AP protocol thread (= `RT_USB_ADAPTER_THREAD_WA_SIZE`)

> **Note:** All builds (release and debug) add 512 extra bytes to each thread
> stack (via `_RT_DEBUG_EXTRA` in `rt_config.h`) to accommodate the 256-byte
> `log_printf_timeout()` buffer used by the runtime logging system. The
> watermark percentages reflect these padded stack sizes.

#### 3. Runtime Heap Status Reporting (Serial Output)

In debug builds, the main loop also prints ChibiOS heap status every ~1 second
alongside the stack watermarks:

```
[HEAP] --- Heap Status ---
[HEAP]  available        : 11800 bytes  (core allocator)
[HEAP]  heap free total  :     0 bytes  (0 fragments)
[HEAP]  integrity        : OK
[HEAP] -----------------------
```

Controlled by the `APP_HEAP_ANALYSIS` compile-time flag (defined in
`app/debug_config.h`). Defaults to `1` in debug builds, `0` otherwise. To
suppress heap output even in a debug build:

```bash
make debug UDEFS="-DAPP_HEAP_ANALYSIS=0"
```

This flag is **independent of the runtime log level** — setting `LOG_LEVEL_NONE`
does not suppress heap status output; only `APP_HEAP_ANALYSIS=0` does.

**Fields:**
- **available** — remaining bytes in the ChibiOS core allocator region (between
  `__heap_base__` and `__heap_end__` from the linker script). This is the actual
  free RAM budget.
- **heap free total** — free bytes within the heap's own free-list, only populated
  after `chHeapAlloc()` calls. Shows fragment count in parentheses (0 = no
  allocations have been made; 1 = single contiguous free block).
- **integrity** — result of `chHeapIntegrityCheck()`. Should always be `OK`;
  `CORRUPT` indicates memory corruption (buffer overflow, use-after-free, etc.).

> **Note:** Currently no application code allocates from the heap at runtime —
> all memory is static. The heap region (~12 KB) acts as a buffer for future
> fixed-allocation growth. This report will become useful when dynamic
> allocations are introduced.

#### 4. Static Stack Usage Analysis (GCC `.su` files)

Debug builds compile with `-fstack-usage`, which generates `.su` (stack usage)
files showing **per-function stack frame sizes**. With LTO enabled, the output
is a single combined file:

```bash
# Show the 20 functions with largest stack usage
sort -t: -k3 -n fw_al1mb1/build/fw_al1mb1.elf.ltrans0.ltrans.su | tail -20
```

Example output:
```
./app/app_log.c:42:20:log_printf_timeout    284    static
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


### Sign App Header (`scripts/build/sign_app_header.sh`)

**NB!** Part of **BUILD!**

Signs the application binary with firmware size and CRC32 checksum for bootloader validation. Called automatically by the Makefile during build.

```bash
./scripts/build/sign_app_header.sh fw_al1mb1/build/fw_al1mb1.bin fw_al1mb1/build/fw_al1mb1_signed.bin
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

Run the memory analysis report after building:

```bash
./scripts/analyse/memory_report.sh fw_al1mb1/build/fw_al1mb1.elf
```

Generates a comprehensive report covering Flash usage, RAM breakdown (fixed vs heap),
BSS symbol analysis by category, thread stack headroom, RAM pressure summary, and
actionable warnings. Use `--no-color` for CI or piped output.

**NB!** The `.heap` section shows all remaining RAM claimed by ChibiOS's core allocator
by design. Currently no code allocates from the heap at runtime — all memory is fixed
at link time. See the report's RAM section for details.


## Attentio Protocol (AP)

The firmware communicates over **CDC1** using the Attentio Protocol (AP), a packet-based interface. The protocol replaces the previous ChibiOS text shell.

**NOTE**: `attentio-cli` is the recommended tool for interacting with the device over USB. Link: https://github.com/engemil/attentio-cli

### Packet Format

```
[SYNC 0xA5] [LEN] [CMD] [PAYLOAD 0-252 bytes] [CRC-8/CCITT]
```

| Field | Size | Description |
|-------|------|-------------|
| SYNC | 1 byte | Always `0xA5` |
| LEN | 1 byte | Length of CMD + PAYLOAD (1-253) |
| CMD | 1 byte | Command ID |
| PAYLOAD | 0-252 bytes | Command-specific data |
| CRC-8 | 1 byte | CRC-8/CCITT (poly 0x07, init 0x00) over LEN+CMD+PAYLOAD |

### Commands

**Host -> Device:**

| Command | ID | Category | Description |
|---------|-----|----------|-------------|
| `CLAIM` | 0x01 | Session | Claim device control session |
| `RELEASE` | 0x02 | Session | Release control session |
| `PING` | 0x03 | Session | Heartbeat / connectivity check |
| `POWER_ON` | 0x10 | Power | Power on the device |
| `POWER_OFF` | 0x11 | Power | Power off the device |
| `LED_OFF` | 0x20 | LED | Turn off LED |
| `SET_RGB` | 0x21 | LED | Set LED color (RGB) `[R,G,B]` |
| `SET_HSV` | 0x22 | LED | Set LED color (HSV) `[H:2,S,V]` |
| `SET_BRIGHTNESS` | 0x23 | LED | Set LED brightness `[0-100]` |
| `SET_EFFECT` | 0x30 | Effects | Set LED effect mode (placeholder) |
| `GET_STATUS` | 0x40 | Query | Query device status |
| `GET_METADATA` | 0x43 | Query | Query device metadata (paginated) |
| `METADATA_GET` | 0x44 | Query | Query single metadata field by key |
| `SETTINGS_LIST` | 0x50 | Settings | List all settings |
| `SETTINGS_GET` | 0x51 | Settings | Read a setting value |
| `SETTINGS_SET` | 0x52 | Settings | Write a setting value |
| `LOG_GET_LEVEL` | 0x60 | Log | Query current log level |
| `LOG_SET_LEVEL` | 0x61 | Log | Set runtime log level |
| `DFU_ENTER` | 0x70 | DFU | Reboot into DFU bootloader |

**Device -> Host (Events & Responses):**

| Command | ID | Description |
|---------|-----|-------------|
| `EVT_BUTTON` | 0x80 | Button event (short/long/extended press) |
| `EVT_STATE_CHANGE` | 0x81 | Device state change notification |
| `EVT_SESSION_END` | 0x82 | Session ended (released, takeover, or power off) |
| `OK` | 0xF0 | Success response (optional data payload) |
| `ERROR` | 0xF1 | Error response (1-byte error code) |

**Error Codes** (sent as payload of `ERROR` response):

| Code | Value | Description |
|------|-------|-------------|
| `NONE` | 0x00 | No error |
| `NOT_CONTROLLER` | 0x01 | Sender does not have an active session |
| `INVALID_CMD` | 0x02 | Unknown command ID |
| `INVALID_PARAM` | 0x03 | Invalid parameter value |
| `INVALID_STATE` | 0x04 | Command not valid in current device state |
| `CRC_FAIL` | 0x05 | CRC-8 check failed |

### Session Management (MICB)

The Multi-Interface Control Broker (MICB) provides session-based access control:

- **STANDALONE** mode — device operates autonomously (button-driven state machine)
- **REMOTE** mode — a host has claimed control via `CLAIM` command
- **Session ID** — a `uint16_t` assigned on each `CLAIM` (monotonically incrementing, wraps 65535 → 1, 0 = no session). Returned in the CLAIM OK response (2 bytes, big-endian), included in `GET_STATUS` (bytes 12-13) and `EVT_SESSION_END` (bytes 1-2 after reason)
- Commands that modify device state (LED, power) require an active session
- Query commands (`GET_STATUS`, `GET_METADATA`, `SETTINGS_LIST/GET`) work without a session
- `CLAIM` / `RELEASE` / takeover semantics support future multi-interface control (USB, BLE, WiFi)
- **Button event forwarding** — when in REMOTE mode, physical button presses are forwarded to the controlling host as `EVT_BUTTON` (0x80) packets


## Troubleshooting

Useful CLI tools for debugging/troubleshooting: `minicom`,  `multiarch-gdb`, `usbutils` (`lsusb`)

As well as the already mentioned tools `st-flash`/`st-erase`, `dfu-util`, `openocd`

Compile for debug:
- `make debug` — debug build (`-Og`, disables Stop mode, enables stack checks)

Runtime log level can be changed without recompiling via the `LOG_SET_LEVEL`
AP command (using `attentio-cli` or direct packet).


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
| `IF=00` (`...-if00`) | **CDC0** | Serial print stream (read-only) |
| `IF=02` (`...-if02`) | **CDC1** | Attentio Protocol interface (bidirectional) |

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
