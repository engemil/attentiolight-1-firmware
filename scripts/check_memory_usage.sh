#!/bin/sh
# Memory usage report for STM32C071RB firmware
# Usage: ./memory_usage.sh <elf_file>
# E.g. ./memory_usage.sh build/firmware.elf

if [ -z "$1" ]; then
    echo "Usage: $0 <elf_file>"
    exit 1
fi

ELF_FILE="$1"

if [ ! -f "$ELF_FILE" ]; then
    echo "Error: File '$ELF_FILE' not found"
    exit 1
fi

# STM32C071RB limits (with bootloader)
FLASH_TOTAL=114688  # 112 KB available for app
RAM_TOTAL=24576     # 24 KB

# Get section sizes
SIZES=$(arm-none-eabi-size -A "$ELF_FILE")

# Extract relevant sections (exact match with ^)
TEXT=$(echo "$SIZES" | awk '/^\.text / {print $2}')
RODATA=$(echo "$SIZES" | awk '/^\.rodata / {print $2}')
DATA=$(echo "$SIZES" | awk '/^\.data / {print $2}')
BSS=$(echo "$SIZES" | awk '/^\.bss / {print $2}')
MSTACK=$(echo "$SIZES" | awk '/^\.mstack / {print $2}')
PSTACK=$(echo "$SIZES" | awk '/^\.pstack / {print $2}')
HEAP=$(echo "$SIZES" | awk '/^\.heap / {print $2}')

# Calculate totals
FLASH_USED=$((TEXT + RODATA + DATA))
RAM_STATIC=$((MSTACK + PSTACK + DATA + BSS))

FLASH_PCT=$((FLASH_USED * 100 / FLASH_TOTAL))
RAM_STATIC_PCT=$((RAM_STATIC * 100 / RAM_TOTAL))

echo "=== Memory Usage: $(basename "$ELF_FILE") ==="
echo ""
echo "FLASH: ${FLASH_USED} / ${FLASH_TOTAL} bytes (${FLASH_PCT}%)"
echo "  .text:   ${TEXT} bytes"
echo "  .rodata: ${RODATA} bytes"
echo "  .data:   ${DATA} bytes"
echo ""
echo "RAM (static): ${RAM_STATIC} / ${RAM_TOTAL} bytes (${RAM_STATIC_PCT}%)"
echo "  .mstack: ${MSTACK} bytes"
echo "  .pstack: ${PSTACK} bytes"
echo "  .data:   ${DATA} bytes"
echo "  .bss:    ${BSS} bytes"
echo ""
echo "RAM (heap): ${HEAP} bytes available"
