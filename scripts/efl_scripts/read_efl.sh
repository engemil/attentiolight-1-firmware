#!/bin/bash
################################################################################
# read_efl.sh
# Main CLI tool for reading EFL (Embedded Flash) memory region
#
# Usage: read_efl.sh [OPTIONS]
#
# This tool reads the EFL storage region from STM32C071 using st-flash
# and displays it as a formatted hexadecimal dump.
#
# MIT License - Copyright (c) 2026 EngEmil
################################################################################

set -e

# EFL Memory Configuration (from linker script STM32C071xB_chibios_efl.ld)
EFL_BASE_ADDR="0x0801E000"
EFL_SIZE=8192  # 8KB
EFL_END_ADDR="0x0801FFFF"

# Default values
OFFSET=0
LENGTH=$EFL_SIZE
OUTPUT_FILE=""
FORMAT="hexdump"

# Script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

################################################################################
# Functions
################################################################################

show_help() {
    cat << EOF
EFL Memory Reader - Read and display EFL storage region from STM32C071

Usage: $0 [OPTIONS]

Options:
  --offset <bytes>      Offset from EFL base address (default: 0)
                        Can be decimal (256) or hex (0x100)
  
  --length <bytes>      Number of bytes to read (default: 8192)
                        Can be decimal (256) or hex (0x100)
  
  --output <file>       Save raw binary data to file
                        (hexdump will still be shown to console)
  
  --format <type>       Output format:
                        hexdump - Formatted hex dump with ASCII (default)
                        raw     - Raw binary output only (requires --output)
                        hex     - Hex bytes only, no formatting
  
  --help                Show this help message

EFL Memory Map:
  Base Address:  $EFL_BASE_ADDR
  Size:          $EFL_SIZE bytes (8KB)
  End Address:   $EFL_END_ADDR
  Pages:         4 × 2KB pages

Examples:
  $0
      Read entire EFL region (8KB) and display as hex dump
  
  $0 --offset 0x100 --length 256
      Read 256 bytes starting at offset 0x100 from EFL base
  
  $0 --output efl_backup.bin
      Read entire EFL and save to file (also shows hex dump)
  
  $0 --offset 0 --length 2048 --format hex
      Read first page (2KB) and show hex values only

Notes:
  - Requires ST-Link programmer connected to target
  - st-flash tool must be installed
  - Reading does not modify flash contents
  - Device does not need to be halted for reading

EOF
    exit 0
}

print_error() {
    echo "Error: $1" >&2
    echo "Use --help for usage information" >&2
    exit 1
}

# Convert hex or decimal string to decimal number
to_decimal() {
    local value="$1"
    if [[ "$value" =~ ^0x[0-9A-Fa-f]+$ ]]; then
        printf "%d" "$value"
    elif [[ "$value" =~ ^[0-9]+$ ]]; then
        echo "$value"
    else
        echo "0"
        return 1
    fi
}

################################################################################
# Parse command line arguments
################################################################################

while [ $# -gt 0 ]; do
    case "$1" in
        --offset)
            shift
            OFFSET=$(to_decimal "$1") || print_error "Invalid offset: $1"
            ;;
        --length)
            shift
            LENGTH=$(to_decimal "$1") || print_error "Invalid length: $1"
            ;;
        --output)
            shift
            OUTPUT_FILE="$1"
            ;;
        --format)
            shift
            FORMAT="$1"
            if [[ ! "$FORMAT" =~ ^(hexdump|raw|hex)$ ]]; then
                print_error "Invalid format: $FORMAT (must be hexdump, raw, or hex)"
            fi
            ;;
        --help|-h)
            show_help
            ;;
        *)
            print_error "Unknown option: $1"
            ;;
    esac
    shift
done

################################################################################
# Validate parameters
################################################################################

# Validate offset
if [ $OFFSET -lt 0 ]; then
    print_error "Offset cannot be negative"
fi

if [ $OFFSET -ge $EFL_SIZE ]; then
    print_error "Offset $OFFSET exceeds EFL size $EFL_SIZE"
fi

# Validate length
if [ $LENGTH -le 0 ]; then
    print_error "Length must be greater than zero"
fi

# Check if read extends beyond EFL region
if [ $((OFFSET + LENGTH)) -gt $EFL_SIZE ]; then
    print_error "Read range (offset=$OFFSET, length=$LENGTH) exceeds EFL size $EFL_SIZE"
fi

# Validate format requirements
if [ "$FORMAT" = "raw" ] && [ -z "$OUTPUT_FILE" ]; then
    print_error "Format 'raw' requires --output option"
fi

################################################################################
# Calculate read address
################################################################################

# Convert EFL base address to decimal
EFL_BASE_DEC=$(printf "%d" "$EFL_BASE_ADDR")

# Calculate actual read address
READ_ADDR_DEC=$((EFL_BASE_DEC + OFFSET))
READ_ADDR_HEX=$(printf "0x%08X" $READ_ADDR_DEC)

################################################################################
# Read memory from device
################################################################################

# Create temporary file for read operation
TEMP_FILE=$(mktemp /tmp/efl_dump_XXXXXX.bin)
trap "rm -f $TEMP_FILE" EXIT

echo "================================================================================"
echo "  Reading EFL Memory via ST-Link"
echo "================================================================================"
echo "  EFL Base:      $EFL_BASE_ADDR"
echo "  Read Address:  $READ_ADDR_HEX (offset: $OFFSET bytes)"
echo "  Length:        $LENGTH bytes"
echo "================================================================================"
echo ""

# Call low-level st-flash wrapper
if ! "$SCRIPT_DIR/stflash_read_memory.sh" "$READ_ADDR_HEX" "$LENGTH" "$TEMP_FILE"; then
    print_error "Failed to read memory from device"
fi

echo "Read completed successfully"
echo ""

################################################################################
# Process output based on format
################################################################################

case "$FORMAT" in
    hexdump)
        # Format and display hex dump
        "$SCRIPT_DIR/format_hexdump.sh" "$TEMP_FILE" "$READ_ADDR_HEX"
        
        # Save to file if requested
        if [ -n "$OUTPUT_FILE" ]; then
            cp "$TEMP_FILE" "$OUTPUT_FILE"
            echo "Binary data saved to: $OUTPUT_FILE"
            echo ""
        fi
        ;;
    
    raw)
        # Raw output to file only
        cp "$TEMP_FILE" "$OUTPUT_FILE"
        echo "Raw binary data saved to: $OUTPUT_FILE"
        ;;
    
    hex)
        # Hex values only
        echo "Hex bytes:"
        od -A x -t x1 -v "$TEMP_FILE" | head -n -1
        echo ""
        
        # Save to file if requested
        if [ -n "$OUTPUT_FILE" ]; then
            cp "$TEMP_FILE" "$OUTPUT_FILE"
            echo "Binary data saved to: $OUTPUT_FILE"
            echo ""
        fi
        ;;
esac

exit 0
