#!/bin/bash
################################################################################
# stflash_read_memory.sh
# Low-level wrapper for st-flash memory read operations
#
# Usage: stflash_read_memory.sh <address> <length> <output_file>
#
# MIT License - Copyright (c) 2026 EngEmil
################################################################################

set -e  # Exit on error

# Check arguments
if [ $# -ne 3 ]; then
    echo "Error: Invalid number of arguments" >&2
    echo "Usage: $0 <address> <length> <output_file>" >&2
    echo "Example: $0 0x0801E000 8192 /tmp/efl_dump.bin" >&2
    exit 1
fi

ADDRESS="$1"
LENGTH="$2"
OUTPUT_FILE="$3"

# Validate st-flash is available
if ! command -v st-flash &> /dev/null; then
    echo "Error: st-flash not found. Please install stlink tools." >&2
    exit 1
fi

# Check if ST-Link is connected
if ! st-info --probe &> /dev/null; then
    echo "Error: ST-Link device not detected. Please connect your ST-Link programmer." >&2
    exit 1
fi

# Validate address is a hex number
if [[ ! "$ADDRESS" =~ ^0x[0-9A-Fa-f]+$ ]]; then
    echo "Error: Invalid address format '$ADDRESS'. Use hex format (e.g., 0x0801E000)" >&2
    exit 1
fi

# Validate length is a decimal number
if [[ ! "$LENGTH" =~ ^[0-9]+$ ]]; then
    echo "Error: Invalid length '$LENGTH'. Must be a decimal number." >&2
    exit 1
fi

# Validate length is not zero
if [ "$LENGTH" -eq 0 ]; then
    echo "Error: Length cannot be zero." >&2
    exit 1
fi

# Create output directory if needed
OUTPUT_DIR=$(dirname "$OUTPUT_FILE")
if [ ! -d "$OUTPUT_DIR" ]; then
    mkdir -p "$OUTPUT_DIR"
fi

# Read memory using st-flash
# Note: st-flash requires decimal length, not hex
if ! st-flash read "$OUTPUT_FILE" "$ADDRESS" "$LENGTH" 2>&1; then
    echo "Error: st-flash read operation failed" >&2
    exit 1
fi

# Verify output file was created
if [ ! -f "$OUTPUT_FILE" ]; then
    echo "Error: Output file was not created" >&2
    exit 1
fi

# Verify file size
ACTUAL_SIZE=$(stat -c%s "$OUTPUT_FILE" 2>/dev/null || stat -f%z "$OUTPUT_FILE" 2>/dev/null || echo "0")
if [ "$ACTUAL_SIZE" -ne "$LENGTH" ]; then
    echo "Warning: Expected $LENGTH bytes but got $ACTUAL_SIZE bytes" >&2
fi

exit 0
