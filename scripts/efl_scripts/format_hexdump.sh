#!/bin/bash
################################################################################
# format_hexdump.sh
# Format binary data as readable hexadecimal dump
#
# Usage: format_hexdump.sh <binary_file> <base_address>
#
# Output format (16 bytes per line):
# Address   | Hex Values                                      | ASCII
# 0801E000: DE AD BE EF CA FE BA BE  12 34 56 78 9A BC DE F0 |.........4Vx....|
#
# MIT License - Copyright (c) 2026 EngEmil
################################################################################

set -e

# Check arguments
if [ $# -lt 1 ]; then
    echo "Usage: $0 <binary_file> [base_address]" >&2
    echo "Example: $0 dump.bin 0x0801E000" >&2
    exit 1
fi

INPUT_FILE="$1"
BASE_ADDRESS="${2:-0x0801E000}"  # Default to EFL base

# Validate input file exists
if [ ! -f "$INPUT_FILE" ]; then
    echo "Error: Input file '$INPUT_FILE' not found" >&2
    exit 1
fi

# Convert base address to decimal
BASE_ADDR_DEC=$(printf "%d" "$BASE_ADDRESS")

# Print header
echo "================================================================================"
echo "  EFL Memory Dump"
echo "================================================================================"
echo "  Base Address: $BASE_ADDRESS"
echo "  File Size:    $(stat -c%s "$INPUT_FILE" 2>/dev/null || stat -f%z "$INPUT_FILE" 2>/dev/null) bytes"
echo "================================================================================"
echo ""
echo "Address   | Hex Values                                      | ASCII"
echo "--------------------------------------------------------------------------------"

# Use od to format the binary data
# -A x: Address in hex
# -t x1: One byte per value in hex
# -v: Show all data (no *)
# -w16: 16 bytes per line
od -A n -t x1 -v -w16 "$INPUT_FILE" | {
    OFFSET=0
    while IFS= read -r line; do
        # Skip empty lines
        [ -z "$line" ] && continue
        
        # Calculate address
        ADDR=$((BASE_ADDR_DEC + OFFSET))
        ADDR_HEX=$(printf "%08X" $ADDR)
        
        # Parse hex bytes from od output
        HEX_BYTES=($line)
        
        # Format as two groups of 8 bytes
        HEX_PART=""
        ASCII_PART=""
        
        for i in {0..15}; do
            if [ $i -lt ${#HEX_BYTES[@]} ]; then
                BYTE="${HEX_BYTES[$i]}"
                # Add space after 8th byte
                if [ $i -eq 8 ]; then
                    HEX_PART="$HEX_PART "
                fi
                HEX_PART="$HEX_PART${BYTE^^} "
                
                # Convert to ASCII
                BYTE_DEC=$((16#$BYTE))
                if [ $BYTE_DEC -ge 32 ] && [ $BYTE_DEC -le 126 ]; then
                    ASCII_PART="$ASCII_PART$(printf \\$(printf '%03o' $BYTE_DEC))"
                else
                    ASCII_PART="$ASCII_PART."
                fi
            else
                # Pad with spaces
                if [ $i -eq 8 ]; then
                    HEX_PART="$HEX_PART "
                fi
                HEX_PART="$HEX_PART   "
                ASCII_PART="$ASCII_PART "
            fi
        done
        
        # Print formatted line
        printf "%s: %-49s |%s|\n" "$ADDR_HEX" "$HEX_PART" "$ASCII_PART"
        
        OFFSET=$((OFFSET + 16))
    done
}

echo "--------------------------------------------------------------------------------"
echo ""
exit 0
