# AttentioLight-1 MainBoard-1 Firmware

This is the source code (firmware) for the **AttentioLight-1 MainBoard-1** (`al1mb1`) microcontroller (STM32C071RB).

**Features:**
- Standalone Application, list of modes
    - Solid Color Mode (shared with other modes)
    - Brightness Mode (shared with other modes)
    - Blinking Mode
    - Pulsation Mode
    - Effects Mode (rainbow, color cycle, breathing, candle, fire, lava lamp, Day/Night Cycle, Ocean, Northern Lights, Thunder Storm, Police, Health Pulse, and Memory)
    - Traffic Light Mode (Cycle between Green, Yellow, and Red)
    - Night Light Mode (3 levels of low brightness)
- More to come...


## Standalone Application (and HOW-TO Use it)

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

### Setup Repository

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


## Debug Builds

```bash
cd fw_al1mb1

make                  # Release build (no debug output)
make debug            # Debug build with all outputs
make debug LEVEL=4    # Debug build with all outputs
```

Debug levels (hierarchical):
- `0` = NONE (release)
- `1` = ERROR
- `2` = WARN  
- `3` = INFO
- `4` = DEBUG (default for `make debug`)
- `5` = POWER (verbose + disables Stop mode for debugging)


## Project Structure

(TO DO: To be added)


## Hardware

- Microcontroller: STM32C071RB

(TO DO: Add more info)


## Firmware stack

- ChibiOS
- more..

(TO DO: Add more info)


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
| App Header | 0x08004000 | 32B | Magic, version, size, CRC32 |
| Application | 0x08004100 | ~104KB | Code, vectors, read-only data |
| EFL Storage | 0x0801E000 | 8KB | Persistent settings (4 × 2KB pages) |

### Memory Usage

Check usage after building:

```bash
./scripts/check_memory_usage.sh fw_al1mb1/build/fw_al1mb1.elf
```

**NB!** The `.heap` section by default shows 100% usage of the remaining RAM by design - ChibiOS allocates all remaining RAM to the heap. Hence the dedicated script to check memory usage.


## Libraries and Drivers

- usbcfg and portab (temporary USB related files)

(TO DO move to cfg file and a portability/drivers/library folder?)

**Project-based Libraries/Drivers**
- EngEmil ESP32 Wifi BLE Interface Driver (placeholder, to control enable pin for wireless module)


## Utility Scripts

### EFL Memory Reader (`/workspace/scripts/efl_scripts/`)

CLI tools for reading the EFL (Embedded Flash) storage region via ST-Link debugger.

**Quick usage:**
```bash
# Read entire 8KB EFL region
/workspace/scripts/efl_scripts/read_efl.sh

# Read specific range
/workspace/scripts/efl_scripts/read_efl.sh --offset 0 --length 65

# Save to file
/workspace/scripts/efl_scripts/read_efl.sh --output efl_backup.bin
```

**Features:**
- Hex dump with ASCII representation
- Read any offset/length within EFL region (0x0801E000 - 0x0801FFFF)
- Save to binary file
- No device halt required

See `/workspace/scripts/efl_scripts/README.md` for complete documentation.


## Troubleshooting

Useful CLI tools for debugging/troubleshooting: `minicom`,  `multiarch-gdb`, `usbutils` (`lsusb`)

As well as the already mentioned tools `st-flash`/`st-erase`, `dfu-util`, `openocd`

Use `make debug LEVEL=4` as mentioned, for debug info over serial communication.

(TO DO: Add more info)

Serial Communication, both VCP (through STLINK) and USB.

- `lsusb`
- `ls /dev/ttyACM* /dev/ttyUSB*`
- `minicom -D /dev/ttyACM0 -b 115200`
- `minicom -D /dev/ttyACM1 -b 115200`


**NB!** Bootloader doesn't work so good with the debug setup in VS Code (extensions, etc.). 

<!-- Do the following:
- Change in Makefile:
    ```Makefile
    # uncomment
    LDSCRIPT= $(STARTUPLD)/STM32C071xB.ld
    # Comment out
    #LDSCRIPT= STM32C071xB_bootloader.ld
    ```
- Do not use bootloader, and flash *not signed* firmware bin file with st-flash.
    ```bash
    # Upload application firmware over USB
    cd fw_al1mb1 && make clean && make
    st-flash erase && st-flash --reset write build/fw_al1mb1.bin 0x08000000
     ```
- When you want to use bootloader again:
    - Clean the MCU: `st-flash erase`.
    - Program the bootloader again.
    - Undo change in `Makefile`.
    - Compile application again and flash (with dfu-util).
-->

## Bugs and Issues


### Fixing system timer issues on STM32C071RB ChibiOS port

- Changes in projects files:
    - `cfg/mcuconf.h`
        ```
        #define STM32_ST_USE_TIMER                  16 // TIM17. Note that TIM2 is not working with ChibiOS on STM32C071RB
        ```
    - `cfg/chconf.h`
        ```
        #define CH_CFG_ST_RESOLUTION                16 // Adjust for 16-bit timer. Default was 32-bit.
        ```


### Bug Serial over USB on STM32C071RB

- USB port seem not to work propely. It might fill up a buffer somewhere and freeze the MCU. It remains frozen until you read it.


### Bug first time programming a STM32C071RB

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
