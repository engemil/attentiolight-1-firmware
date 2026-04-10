# Changelog

All notable changes to the EngEmil STM32 Bootloader project will be documented in this file.

**Version Format:** MAJOR.MINOR.PATCH
- **MAJOR:** Incompatible API/protocol changes
- **MINOR:** New features (backward compatible)
- **PATCH:** Bug fixes (backward compatible)

[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

Note: Update `app_header.h` when publishing new version.

---

## [Development] (2026-04-10)

Added
- **Expanded `GET_METADATA` (0x43) with pagination and new fields** — metadata
  response now uses paginated wire format: `[total_count][page][page_count][KV pairs]`.
  Request payload `[page:1]` selects page number; empty payload defaults to page 0
  (backwards-compatible intent). Total metadata fields expanded from 7 to 15:
  - `protocol_version` — AP protocol version string
  - `uptime` — device uptime in seconds (string)
  - `build_time` — firmware build time (HH:MM:SS)
  - `hardware_revision` — board hardware revision from BSP
  - `compiler` — compiler name and version
  - `chibios_port_info` — ChibiOS port info string
  - `chip_uid` — STM32 96-bit unique ID (24 hex chars)
  - `serial_number` — alias for `chip_uid` (same value)
- **New AP command `METADATA_GET` (0x44)** — single-field metadata query, mirroring
  the `SETTINGS_GET` (0x51) pattern. Request: `[key_len:1][key:N]`, response:
  `[key_len:1][key:N][val_len:1][val:N]`. Returns `ERR_INVALID_PARAM` for unknown keys.
- **Metadata helper functions** — `md_entry_t` type, `metadata_build_table()`,
  `metadata_find()`, `u8_to_dec()`, `u32_to_dec()` utilities in `micb.c`. All marked
  `__attribute__((noinline))` to prevent LTO stack inflation (see Fixed below).

Changed
- **`GET_METADATA` wire format breaking change** — response format changed from
  `[count][KV pairs]` to `[total_count][page][page_count][KV pairs]`. Both firmware
  and CLI repos are updated together.

Fixed
- **LTO-induced HardFault from metadata functions** — with `-O2` and LTO enabled,
  the new metadata functions (allocating `md_entry_t entries[15]` + `char scratch[128]`
  on stack) were inlined into the USB adapter thread's call chain via
  `micb_process_command()`, pushing the 768-byte stack past its limit. The overflow
  corrupted an adjacent thread's saved context, causing `__port_switch` to restore
  PC=0x00000000 and HardFault on every boot. Fixed by marking all metadata functions
  `__attribute__((noinline))` and using a shared file-scope `md_payload[]` buffer.
  Zero net BSS increase (BSS remains 0x2988 = 10,632 bytes, identical to baseline).

Added
- **`scripts/analyse/memory_report.sh`** — comprehensive POSIX shell memory analysis
  tool for the firmware ELF. Generates flash usage with full-chip proportional map
  (bootloader / header / app / EFL regions), RAM overview (fixed vs heap), BSS
  breakdown by category, thread stack headroom analysis with LTO warnings, RAM
  pressure summary, and actionable recommendations. Replaces the old
  `scripts/check_memory_usage.sh`. Supports `--no-color` for CI and `--map` override.
- **`APP_STACK_WATERMARK` compile-time flag** — dedicated flag controlling runtime
  thread stack watermark reporting, replacing the previous `APP_DEBUG_BUILD` guard.
  Defaults to 1 in debug builds, 0 otherwise. Can be overridden with
  `-DAPP_STACK_WATERMARK=0` to suppress watermark output even in debug builds.
- **`APP_HEAP_ANALYSIS` compile-time flag** — periodic ChibiOS heap status reporting
  (core free memory, heap fragmentation, integrity check). Same pattern as
  `APP_STACK_WATERMARK`: defaults to 1 in debug builds, suppressible with
  `-DAPP_HEAP_ANALYSIS=0`. Currently the heap is idle (no runtime allocations);
  this report is infrastructure for future dynamic allocation monitoring.
- **`debug_config.h`** — new header centralizing compile-time debug diagnostic
  flags (`APP_STACK_WATERMARK`, `APP_HEAP_ANALYSIS`). These flags are independent
  of the runtime log level — `LOG_LEVEL_NONE` does not suppress diagnostic output.
  Sits alongside `rt_config.h` as a companion configuration header.

Changed
- **Stack watermark output format** — updated `[STACK]` delimiters from `===` to
  `---` style. Documentation now correctly describes the feature as reporting every
  ~1 s (was incorrectly documented as 5 s) and not gated by log level.

Removed
- **`scripts/check_memory_usage.sh`** — superseded by `scripts/analyse/memory_report.sh`.

- **Attentio Protocol (AP)** interface on CDC1, replacing the ChibiOS
  text-based shell. Framed packet format: `[SYNC 0xA5][LEN][CMD][PAYLOAD][CRC-8/CCITT]`.
  Supports 20+ commands across session, LED control, power, query, settings,
  log control, and DFU categories.
- **Multi-Interface Control Broker (MICB)** (`micb.c/h`) — session-based access
  control with STANDALONE and REMOTE modes. Routes AP commands to handlers,
  enforces claim-based access control, and coordinates mode transitions with
  the animation engine and state machine. Supports claim/release/takeover
  semantics for multi-interface (USB, future BLE/WiFi) control.
- **USB adapter** (`usb_adapter.c/h`) — AP packet parser thread on CDC1 that
  bridges USB serial to MICB command processing. Replaces the ChibiOS shell
  thread.
- **Runtime logging system** (`app_log.c/h`) — replaces compile-time `DBG_*`
  macros (`app_debug.h`, removed) with runtime `LOG_*` macros (ERROR, WARN,
  INFO, DEBUG). All log code is always compiled in; filtering happens at
  runtime via a global log level variable. Boot default loaded from persistent
  storage, overridable at runtime via AP command without reboot.
- **AP commands**: `CLAIM`, `RELEASE`, `PING`, `POWER_ON`, `POWER_OFF`,
  `LED_OFF`, `SET_RGB`, `SET_HSV`, `SET_BRIGHTNESS`, `SET_EFFECT`,
  `GET_STATE`, `GET_CAPS`, `GET_SESSION`, `GET_METADATA`, `SETTINGS_LIST`,
  `SETTINGS_GET`, `SETTINGS_SET`, `LOG_GET_LEVEL`, `LOG_SET_LEVEL`,
  `DFU_ENTER`.
- **Button event forwarding in REMOTE mode** — when a remote host has
  claimed control, physical button presses are forwarded as `EVT_BUTTON`
  (0x80) AP packets to the controlling host.
- `log_level` field in persistent data (`pd_data_t`), saved to EFL and
  restored on boot.
- `_RT_DEBUG_EXTRA` (512 bytes) now applied unconditionally to all thread
  stacks (both release and debug builds) to accommodate the 256-byte
  `log_printf_timeout()` stack buffer.

Changed
- **ChibiOS shell removed** — all `cmd_*.c/h` files deleted, `shellconf.h`
  deleted, `shell.mk` removed from Makefile. CDC1 is now exclusively used
  by the AP binary protocol via the USB adapter thread.
- **Logging refactored** — `app_debug.h` (compile-time `DBG_*` macros)
  replaced by `app_log.h` (runtime `LOG_*` macros). All source files updated
  to use `LOG_ERROR`/`LOG_WARN`/`LOG_INFO`/`LOG_DEBUG` instead of
  `DBG_ERROR`/`DBG_WARN`/`DBG_INFO`/`DBG_DEBUG`.
- **Persistent data version bumped to `0x0003`** — new `log_level` field
  added. EFL storage page changed from page 0 to page 1. Existing settings
  will reset to defaults on firmware upgrade.
- **Debug build** (`make debug`): now adds `-DAPP_DEBUG_BUILD=1` which
  enables ChibiOS stack checks (`CH_DBG_ENABLE_STACK_CHECK`), stack fill
  patterns (`CH_DBG_FILL_THREADS`), and generates `.su` files for
  per-function stack analysis. Also disables Stop mode.
- Main thread loop simplified from shell respawn logic to idle sleep with
  optional stack watermark reporting (debug builds).
- **Standalone code separated** — modes, system states, and standalone-specific
  globals moved into `app_state_machine/standalone/`. Renamed `global_brightness`
  → `standalone_brightness`, `global_color_index` → `standalone_color_index`,
  `shared_color_palette` → `standalone_color_palette`. Extracted standalone state
  variables into `standalone_state.c/h`. Renamed `app_state_machine_config.h` →
  `standalone_config.h`. Prepares the codebase for clean separation between
  standalone and remote control paths.

Fixed
- **Stack overflow causing HardFault in release builds** — several functions
  in `micb.c` allocated 252-256 byte arrays (`payload[AP_MAX_PAYLOAD_SIZE]`,
  `evt_buf[AP_MAX_PACKET_SIZE]`) on the stack. Combined with the 256-byte
  buffer from `log_printf_timeout()` and thread frame overhead, the USB
  adapter thread's 768-byte stack overflowed under `-O2` optimization,
  corrupting ChibiOS kernel data structures. Fixed by making 5 large
  stack-local buffers `static` in `micb.c` (moved ~1268 bytes from stack
  to BSS).
- **Race condition in `app_sm_start()`** — `driver_state` was set to
  `APP_SM_RUNNING` after `chThdCreateStatic()`, meaning the new thread
  could run and check `driver_state` before it was updated. Fixed by
  setting `driver_state` before thread creation.
- **`input_names[]` array missing `MODE_TRANSITION_COMPLETE` entry** —
  caused out-of-bounds read when logging that input event.
- **`cmd_power_off()` sent wrong event** — sent `APP_SM_INPUT_BTN_EXTENDED_RELEASE`
  instead of `APP_SM_INPUT_BTN_EXTENDED_START`, preventing power off from
  working via AP command.

## [Development] (2026-03-29)

Added
- **USB serial number from chip UID** — The USB iSerialNumber descriptor now
  reports the STM32C071RB 96-bit hardware unique ID (`UID_BASE` at `0x1FFF7550`)
  formatted as 24 uppercase hex characters. Both bootloader (DFU) and application
  (Normal) modes report the same immutable serial, enabling reliable device
  identity across USB re-enumeration and firmware updates.
- **Extended metadata fields** — The `metadata` command now includes ChibiOS system information (previously from the built-in `info` command):
  - `chibios_kernel` — ChibiOS kernel version
  - `compiler` — Compiler name and version
  - `architecture` — CPU architecture (e.g., "ARMv6-M")
  - `core_variant` — Core variant (e.g., "Cortex-M0+")
  - `chibios_port_info` — ChibiOS port info
  - `platform` — MCU platform name
  - `board` — Board name from BSP
- **Shell command: `metadata`** — Read-only device identity and build information:
  - `metadata` or `metadata list` — Lists all metadata fields in `key=value` format
  - `metadata get <key>` — Reads a specific field value
  - Fields: `serial_number` (EFL), `firmware_version`, `device_model`, `hardware_revision`, `build_date`, `chip_uid`
- **Separate EFL storage for metadata and settings** — Device metadata stored in EFL page 0 (production-programmed, preserved on factory reset), user settings in EFL page 1 (user-configurable, reset to defaults on factory reset).
- **Device metadata module** (`device_metadata.c/h`) — Manages read-only production data with magic `0x4D444154` ("MDAT") and CRC32 validation.
- **Compile-time defines** `DEVICE_MODEL` ("AttentioLight-1") and `HARDWARE_REVISION` ("Rev C") moved from `app_header.h` to `board.h` for better hardware/firmware separation.
- **ChibiOS Shell integration** on CDC1 (PORTAB_SDU2). Shell thread is dynamically allocated from heap and respawns on USB reconnection for graceful cable plug/unplug handling.
- **Shell command: `version`** — Returns firmware version (`<major>.<minor>.<patch>`) read from the application header struct.
- **Shell command: `settings get/set`** — Read and write persistent data fields by name string. Uses `persistent_data_find_field_by_name()` for dynamic field lookup. Respects access control (RO/RW per field).
- **Shell command: `dfu`** — Writes magic value `0xDEADBEEF` to RAM address `0x20005FFC` and triggers `NVIC_SystemReset()` to enter bootloader DFU mode. No response sent (device reboots immediately).
- **Shell response helper macros** (`shell_helpers.h`): `shell_ok(chp)`, `shell_error(chp, msg)`, `shell_value(chp, val)` for consistent `OK\r\n` / `ERROR ...\r\n` protocol formatting.
- `persistent_data_find_field_by_name()` function for case-sensitive string-based field lookup in the persistent data registry.
- Shell thread priority (`RT_SHELL_THREAD_PRIORITY`) and working area size (`SHELL_WA_SIZE`) definitions in `rt_config.h`.
- `shellconf.h` with custom shell prompt and disabled `test` and `info` built-in commands (info replaced by extended `metadata` command).
- `extern` declaration for `app_header` in `app_header.h` (used by `cmd_version`).

Changed
- **`serial_number` now returns chip UID** — `metadata get serial_number` returns the
  STM32 96-bit hardware UID (same value as `chip_uid`) instead of reading from EFL.
  The `serial_number` field in `device_metadata_t` has been replaced with a `_reserved`
  placeholder to preserve struct layout, but is no longer used.
- **`serial_number` moved from settings to metadata** — Now a read-only field queried via `metadata get serial_number` instead of `settings get serial_number`. The `settings` command no longer exposes `serial_number`.
- **Simplified persistent data API** — Removed generic field registry (`persistent_data_find_field_by_name()`, `pd_field_id_t`, `pd_access_t`, etc.). Only `device_name` remains as a writable setting via direct setter `persistent_data_set_device_name()`.
- **Settings version bumped to `0x0002`** — Incompatible with v1 format due to `serial_number` removal; existing settings will reset to defaults on firmware upgrade.
- `device_name` persistent data field access changed from `PD_ACCESS_RO` to `PD_ACCESS_RW` (writable over USB shell).
- Debug print timeout increased from 50ms to 200ms (`DBG_PRINT_TIMEOUT_MS` in `app_debug.h`).
- Main thread loop refactored from simple sleep to shell respawn loop: checks USB active state, creates shell thread from heap, waits for termination, then re-spawns.
- Main loop sleep interval changed from 5000ms to 1000ms (faster shell respawn on reconnect).
- Makefile updated: added ChibiOS shell module (`shell.mk`), shell command source files, `SHELL_CONFIG_FILE` define, and `shell_commands/` include path.
- **Shell command `settings` enhanced** — now supports `settings list` (or just `settings` with no arguments) to output all settings in `key=value` format. Skips INTERNAL access fields and only shows user-accessible RO/RW settings. This enables the CLI to retrieve all settings for export/preset functionality.

Added (previous)
- **Dual USB CDC/ACM** with IAD (Interface Association Descriptor) support. The device now enumerates two virtual serial ports over a single USB connection:
  - **CDC0** (`PORTAB_SDU1`): Debug print stream (device to host, read-only).
  - **CDC1** (`PORTAB_SDU2`): Shell command interface (bidirectional, for future CLI integration).
- Second `SerialUSBDriver` (`PORTAB_SDU2` / `SDU2`) and corresponding `SerialUSBConfig` (`serusbcfg2`).
- USB endpoints EP3 (interrupt IN) and EP4 (bulk IN+OUT) for CDC1.
- `SET_INTERFACE` request handling in USB requests hook (required for composite devices).

Changed
- USB device descriptor bumped from USB 1.1 (`bcdUSB 0x0110`) to USB 2.0 (`bcdUSB 0x0200`), required for IAD.
- USB device class changed from CDC (`0x02`) to Miscellaneous (`0xEF`) with Common Class subclass (`0x02`) and IAD protocol (`0x01`).
- USB configuration descriptor expanded from single CDC (67 bytes, 2 interfaces) to dual CDC with IAD (141 bytes, 4 interfaces).
- Renamed `serusbcfg` to `serusbcfg1` across all source files (`usbcfg.c`, `usbcfg.h`, `main.c`, `state_off.c`).
- Updated `portab.h` with `PORTAB_SDU2` definition.

---

## [1.1.0] (2026-03-04)

Added
- **Thread stack analysis tools** for debug builds:
  - Runtime stack overflow detection (`CH_DBG_ENABLE_STACK_CHECK`, auto-enabled in debug builds).
  - Runtime stack watermark reporting via `dbg_print_stack_usage()` (opt-in via `DBG_ENABLE_STACK_WATERMARK`).
  - Static stack usage analysis via GCC `-fstack-usage` flag (generates `.su` files in debug builds).
- Dynamic debug stack padding (`_RT_DEBUG_EXTRA`): adds 512 bytes to all thread stacks in debug builds to accommodate the 256-byte printf buffer.
- Auto-enable `CH_DBG_FILL_THREADS` in debug builds for fill-pattern stack analysis.
- "Erase Flash (st-flash)" step added to One-Shot VSCode tasks.
- README documentation for `sign_app_header.sh`, system setup scripts, and thread stack analysis.
- Alternative optimization for debugging.
- Warnings about max (3-4?) hardware breakpoints.

Fixed
- Fixed debugging in VS Code (.vscode/launch.json-file).

Changed
- README restructured: reordered sections, expanded Table of Contents with nested entries.
- All script paths in README and EFL scripts README changed from absolute to relative paths.
- Main loop sleep interval changed from 500ms to 5000ms.
- Removed test/commented-out code from `main.c`.
- VS Code tasks improved.
- Makefile comment tweaks.
- Updated bootloader submodule.
- Cleanup of vscode files.
- Updated bootloader submodule.

Removed
- `scripts/system/udevusb_esp.sh` (ESP-specific udev rules, not used in this project).

---

## [1.0.1] (2026-02-24)

Fixed
- Fixed problem with USB serial com. causing thread(s) to freeze. Change debug prints for a timeout alternative.

Changed
- Minor tweak in udev stlink script.
- Added comments about timer in chibios config files.
- Removed bugs notes in README.

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


