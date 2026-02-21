# EFL Memory Reader Scripts

CLI tools for reading and displaying the EFL (Embedded Flash) storage region from STM32C071 microcontroller.

## Overview

These scripts provide a convenient command-line interface to read flash memory from the STM32C071's EFL storage region using ST-Link programmer and st-flash utility.

### EFL Memory Region Configuration

Based on linker script `STM32C071xB_chibios_efl.ld`:

- **Base Address**: `0x0801E000`
- **Size**: `8192 bytes` (8KB)
- **End Address**: `0x0801FFFF`
- **Organization**: 4 pages × 2KB per page
- **Location**: Last 8KB of 128KB flash memory

## Requirements

### Hardware
- STM32C071 microcontroller
- ST-Link V2/V3 programmer connected via SWD

### Software
- `st-flash` (stlink tools) - [Install Instructions](https://github.com/stlink-org/stlink)
- `bash` shell
- `od` utility (standard on most Linux systems)

## Installation

The scripts are located in `/workspace/scripts/efl_scripts/` and are already executable.

Verify st-flash is installed:
```bash
st-flash --version
```

Check ST-Link connection:
```bash
st-info --probe
```

Expected output:
```
Found 1 stlink programmers
  version:    V3J16
  serial:     0040003D3232511639353236
  flash:      131072 (pagesize: 2048)
  sram:       24576
  chipid:     0x493
  dev-type:   STM32C071xx
```

## Usage

### Quick Start

Read and display entire EFL region (8KB):
```bash
cd /workspace/scripts/efl_scripts
./read_efl.sh
```

### Basic Examples

**Read entire EFL region:**
```bash
/workspace/scripts/efl_scripts/read_efl.sh
```

**Read specific range (offset 256 bytes, length 128 bytes):**
```bash
/workspace/scripts/efl_scripts/read_efl.sh --offset 256 --length 128
```

**Read first page (2KB):**
```bash
/workspace/scripts/efl_scripts/read_efl.sh --offset 0 --length 2048
```

**Read using hex values:**
```bash
/workspace/scripts/efl_scripts/read_efl.sh --offset 0x100 --length 0x200
```

### Saving to File

**Save entire EFL to binary file:**
```bash
/workspace/scripts/efl_scripts/read_efl.sh --output efl_backup.bin
```

**Save partial region:**
```bash
/workspace/scripts/efl_scripts/read_efl.sh --offset 0x100 --length 512 --output partial.bin
```

### Output Formats

**Hexdump format (default):**
```bash
/workspace/scripts/efl_scripts/read_efl.sh --format hexdump
```
Output example:
```
Address   | Hex Values                                      | ASCII
--------------------------------------------------------------------------------
0801E000: DE AD BE EF CA FE BA BE  12 34 56 78 9A BC DE F0 |.........4Vx....|
0801E010: A5 5A F0 0F 55 AA 33 CC  01 02 03 04 05 06 07 08 |.Z..U.3.........|
```

**Hex values only:**
```bash
/workspace/scripts/efl_scripts/read_efl.sh --format hex
```

**Raw binary (requires --output):**
```bash
/workspace/scripts/efl_scripts/read_efl.sh --format raw --output data.bin
```

## Command Reference

### read_efl.sh

Main CLI tool for reading EFL memory.

```bash
/workspace/scripts/efl_scripts/read_efl.sh [OPTIONS]
```

#### Options

| Option | Argument | Description |
|--------|----------|-------------|
| `--offset` | `<bytes>` | Offset from EFL base (decimal or hex, default: 0) |
| `--length` | `<bytes>` | Number of bytes to read (decimal or hex, default: 8192) |
| `--output` | `<file>` | Save binary data to file |
| `--format` | `hexdump\|raw\|hex` | Output format (default: hexdump) |
| `--help` | - | Show help message |

#### Examples

```bash
# Read entire region
/workspace/scripts/efl_scripts/read_efl.sh

# Read 256 bytes from offset 0x100
/workspace/scripts/efl_scripts/read_efl.sh --offset 0x100 --length 256

# Save entire region to file
/workspace/scripts/efl_scripts/read_efl.sh --output backup_$(date +%Y%m%d).bin

# Read and show hex only
/workspace/scripts/efl_scripts/read_efl.sh --length 64 --format hex
```

### stflash_read_memory.sh

Low-level wrapper for st-flash read operations (typically not called directly).

```bash
/workspace/scripts/efl_scripts/stflash_read_memory.sh <address> <length> <output_file>
```

Example:
```bash
/workspace/scripts/efl_scripts/stflash_read_memory.sh 0x0801E000 8192 /tmp/efl.bin
```

### format_hexdump.sh

Format binary file as hexadecimal dump (typically not called directly).

```bash
/workspace/scripts/efl_scripts/format_hexdump.sh <binary_file> [base_address]
```

Example:
```bash
/workspace/scripts/efl_scripts/format_hexdump.sh /tmp/efl.bin 0x0801E000
```

## Troubleshooting

### ST-Link not detected

**Error:**
```
Error: ST-Link device not detected. Please connect your ST-Link programmer.
```

**Solutions:**
1. Check USB connection
2. Try reconnecting ST-Link
3. Check if another tool is using ST-Link:
   ```bash
   sudo killall st-util openocd
   ```
4. Try with sudo (permissions issue):
   ```bash
   sudo ./scripts/read_efl.sh
   ```

### st-flash not found

**Error:**
```
Error: st-flash not found. Please install stlink tools.
```

**Solution:** Install stlink tools:
```bash
# Ubuntu/Debian
sudo apt-get install stlink-tools

# From source
git clone https://github.com/stlink-org/stlink.git
cd stlink
make
sudo make install
```

### Invalid address or range

**Error:**
```
Error: Read range exceeds EFL size
```

**Solution:** Verify your offset and length are within EFL bounds:
- Maximum offset: `8191` (or `0x1FFF`)
- Maximum length: `8192 - offset`

### Permission denied

**Error:**
```
Permission denied
```

**Solutions:**
1. Make scripts executable:
   ```bash
   chmod +x ./scripts/*.sh
   ```
2. Add user to plugdev group (for ST-Link access):
   ```bash
   sudo usermod -a -G plugdev $USER
   # Log out and back in
   ```

## Technical Details

### Memory Read Process

1. **Validate parameters** - Check offset and length are within EFL bounds
2. **Calculate address** - Add offset to EFL base address (0x0801E000)
3. **Read via st-flash** - Execute `st-flash read` command
4. **Format output** - Parse binary data and format as hex dump
5. **Display/save** - Show to console and/or save to file

### Script Architecture

```
read_efl.sh (Main CLI)
    ↓
    ├─→ stflash_read_memory.sh (Read from device)
    │       └─→ st-flash (Hardware interface)
    │
    └─→ format_hexdump.sh (Format output)
            └─→ od (Binary to hex conversion)
```

### Safety Features

- **Read-only operation** - Scripts only read memory, never write
- **Bounds checking** - Validates all addresses are within EFL region
- **Error handling** - Proper error messages and exit codes
- **No device halt** - Reading doesn't interfere with running firmware

## Related Files

- `STM32C071xB_chibios_efl.ld` - Linker script defining EFL region
- `libs/hal_efl_stm32c0xx/efl_storage.h` - EFL storage helper macros
- `libs/hal_efl_stm32c0xx/efl_test.c` - EFL driver test code
- `app/main.c` - Application using EFL storage

## Use Cases

### 1. Verify EFL Test Results
After running the EFL test in firmware, verify data was written correctly:
```bash
/workspace/scripts/efl_scripts/read_efl.sh --length 64
```

### 2. Backup EFL Data
Create a backup before firmware updates:
```bash
/workspace/scripts/efl_scripts/read_efl.sh --output efl_backup_$(date +%Y%m%d_%H%M%S).bin
```

### 3. Debug Storage Issues
Check specific addresses where data should be stored:
```bash
/workspace/scripts/efl_scripts/read_efl.sh --offset 0x100 --length 256
```

### 4. Verify Erase Operations
Check if a sector was properly erased (should show 0xFF):
```bash
/workspace/scripts/efl_scripts/read_efl.sh --offset 0 --length 2048
```
