#!/bin/sh
# =============================================================================
# Memory Report for STM32C071RB Firmware
# =============================================================================
#
# Generates a comprehensive, color-coded memory analysis report from a built
# ELF file. Shows Flash usage, RAM breakdown, BSS symbol analysis, thread
# stack headroom, RAM pressure, and actionable warnings.
#
# Usage:
#   ./scripts/analyse/memory_report.sh <elf_file> [options]
#
# Options:
#   --no-color       Disable ANSI color output (for CI/pipes)
#   --map <file>     Path to .map file (auto-detected by default)
#
# Example:
#   ./scripts/analyse/memory_report.sh fw_al1mb1/build/fw_al1mb1.elf
#
# Requirements:
#   arm-none-eabi-size, arm-none-eabi-nm (from ARM GCC toolchain)
#
# =============================================================================

set -e

# ─── Target MCU constants ───────────────────────────────────────────────────
MCU_NAME="STM32C071RB"
MCU_FLASH_TOTAL=131072          # 128 KB total chip flash
MCU_RAM_TOTAL=24576             # 24 KB total SRAM
BOOTLOADER_SIZE=16384           # 16 KB bootloader
APP_HEADER_SIZE=256             # 32B header + 224B padding (aligned)
EFL_SIZE=8192                   # 8 KB EFL storage (4 x 2KB pages)
# App flash = total - bootloader - app_header - EFL
APP_FLASH_TOTAL=$((MCU_FLASH_TOTAL - BOOTLOADER_SIZE - APP_HEADER_SIZE - EFL_SIZE))

# ChibiOS thread_t overhead on Cortex-M0+ (measured from nm: WA=984, declared=768 -> 216)
THREAD_OVERHEAD=216

# LOG_* printf buffer size (always on calling thread's stack)
LOG_BUF_SIZE=256

# Bar chart width
BAR_WIDTH=40

# ─── Color setup ────────────────────────────────────────────────────────────
USE_COLOR=1

setup_colors() {
    if [ "$USE_COLOR" = "1" ] && [ -t 1 ]; then
        BOLD="\033[1m"
        DIM="\033[2m"
        RESET="\033[0m"
        RED="\033[31m"
        GREEN="\033[32m"
        YELLOW="\033[33m"
        BLUE="\033[34m"
        CYAN="\033[36m"
        WHITE="\033[37m"
        BOLD_WHITE="\033[1;37m"
        BOLD_RED="\033[1;31m"
        BOLD_YELLOW="\033[1;33m"
        BOLD_GREEN="\033[1;32m"
        BOLD_CYAN="\033[1;36m"
        BG_RED="\033[41m"
        BG_YELLOW="\033[43m"
        BG_GREEN="\033[42m"
    else
        BOLD="" DIM="" RESET=""
        RED="" GREEN="" YELLOW="" BLUE="" CYAN="" WHITE=""
        BOLD_WHITE="" BOLD_RED="" BOLD_YELLOW="" BOLD_GREEN="" BOLD_CYAN=""
        BG_RED="" BG_YELLOW="" BG_GREEN=""
    fi
}

# ─── Argument parsing ───────────────────────────────────────────────────────
ELF_FILE=""
MAP_FILE=""

while [ $# -gt 0 ]; do
    case "$1" in
        --no-color)
            USE_COLOR=0
            shift
            ;;
        --map)
            MAP_FILE="$2"
            shift 2
            ;;
        -h|--help)
            echo "Usage: $0 <elf_file> [--no-color] [--map <file>]"
            echo ""
            echo "Generates a comprehensive memory analysis report for STM32C071RB firmware."
            echo ""
            echo "Options:"
            echo "  --no-color    Disable ANSI colors (for CI/pipes)"
            echo "  --map <file>  Path to .map file (auto-detected from ELF path)"
            echo "  -h, --help    Show this help"
            exit 0
            ;;
        -*)
            echo "Unknown option: $1" >&2
            exit 1
            ;;
        *)
            if [ -z "$ELF_FILE" ]; then
                ELF_FILE="$1"
            else
                echo "Error: unexpected argument '$1'" >&2
                exit 1
            fi
            shift
            ;;
    esac
done

if [ -z "$ELF_FILE" ]; then
    echo "Usage: $0 <elf_file> [--no-color] [--map <file>]" >&2
    exit 1
fi

if [ ! -f "$ELF_FILE" ]; then
    echo "Error: ELF file '$ELF_FILE' not found" >&2
    exit 1
fi

# Auto-detect map file
if [ -z "$MAP_FILE" ]; then
    ELF_DIR=$(dirname "$ELF_FILE")
    ELF_BASE=$(basename "$ELF_FILE" .elf)
    MAP_FILE="${ELF_DIR}/${ELF_BASE}.map"
fi

# Check tools
for tool in arm-none-eabi-size arm-none-eabi-nm; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "Error: $tool not found in PATH" >&2
        exit 1
    fi
done

setup_colors

# ─── Helper functions ───────────────────────────────────────────────────────

# Format number with comma separators: 12756 -> "12,756"
fmt_num() {
    echo "$1" | awk '{
        n = $1
        if (n < 0) { sign = "-"; n = -n } else { sign = "" }
        s = sprintf("%d", n)
        len = length(s)
        result = ""
        for (i = 1; i <= len; i++) {
            if (i > 1 && (len - i + 1) % 3 == 0) result = result ","
            result = result substr(s, i, 1)
        }
        printf "%s%s", sign, result
    }'
}

# Print a bar: bar <used> <total> <width>
# Returns the bar string and sets BAR_COLOR based on percentage
print_bar() {
    _used=$1
    _total=$2
    _width=$3
    if [ "$_total" -eq 0 ]; then
        _pct=0
    else
        _pct=$((_used * 100 / _total))
    fi
    _filled=$((_used * _width / _total))
    if [ "$_filled" -gt "$_width" ]; then
        _filled=$_width
    fi
    _empty=$((_width - _filled))

    # Color based on percentage
    if [ "$_pct" -ge 90 ]; then
        _color="$RED"
    elif [ "$_pct" -ge 75 ]; then
        _color="$YELLOW"
    else
        _color="$GREEN"
    fi

    # Build bar
    _bar=""
    _i=0
    while [ "$_i" -lt "$_filled" ]; do
        _bar="${_bar}█"
        _i=$((_i + 1))
    done
    _i=0
    while [ "$_i" -lt "$_empty" ]; do
        _bar="${_bar}░"
        _i=$((_i + 1))
    done

    printf "%b%s%b" "$_color" "$_bar" "$RESET"
}

# Print percentage with color
pct_color() {
    _pct=$1
    if [ "$_pct" -ge 90 ]; then
        printf "%b%3d%%%b" "$BOLD_RED" "$_pct" "$RESET"
    elif [ "$_pct" -ge 75 ]; then
        printf "%b%3d%%%b" "$BOLD_YELLOW" "$_pct" "$RESET"
    else
        printf "%b%3d%%%b" "$GREEN" "$_pct" "$RESET"
    fi
}

# Print a section header
section_header() {
    printf "\n%b── %s %b" "$BOLD_CYAN" "$1" "$RESET"
    # Fill remaining width with ─
    _len=${#1}
    _remaining=$((68 - _len - 4))
    _i=0
    while [ "$_i" -lt "$_remaining" ]; do
        printf "─"
        _i=$((_i + 1))
    done
    printf "\n\n"
}

# Print a row: row <label> <value_bytes> <pct> <note>
row() {
    _label=$1
    _bytes=$2
    _pct=$3
    _note=$4
    _fmt_bytes=$(fmt_num "$_bytes")
    if [ -n "$_note" ]; then
        printf "  %-28s %8s  %s  %b%s%b\n" "$_label" "$_fmt_bytes" "$_pct" "$DIM" "$_note" "$RESET"
    else
        printf "  %-28s %8s  %s\n" "$_label" "$_fmt_bytes" "$_pct"
    fi
}

# Separator line
sep() {
    printf "  %b" "$DIM"
    _i=0
    while [ "$_i" -lt 60 ]; do
        printf "─"
        _i=$((_i + 1))
    done
    printf "%b\n" "$RESET"
}

# ─── Extract section sizes ──────────────────────────────────────────────────

SIZES=$(arm-none-eabi-size -A "$ELF_FILE")

extract_section() {
    echo "$SIZES" | awk -v sec="$1" '$1 == sec {print $2}'
}

SEC_TEXT=$(extract_section ".text")
SEC_RODATA=$(extract_section ".rodata")
SEC_DATA=$(extract_section ".data")
SEC_BSS=$(extract_section ".bss")
SEC_MSTACK=$(extract_section ".mstack")
SEC_PSTACK=$(extract_section ".pstack")
SEC_HEAP=$(extract_section ".heap")
SEC_VECTORS=$(extract_section ".vectors")
SEC_APP_HEADER=$(extract_section ".app_header")
SEC_EXIDX=$(extract_section ".ARM.exidx")

# Default to 0 if not found
SEC_TEXT=${SEC_TEXT:-0}
SEC_RODATA=${SEC_RODATA:-0}
SEC_DATA=${SEC_DATA:-0}
SEC_BSS=${SEC_BSS:-0}
SEC_MSTACK=${SEC_MSTACK:-0}
SEC_PSTACK=${SEC_PSTACK:-0}
SEC_HEAP=${SEC_HEAP:-0}
SEC_VECTORS=${SEC_VECTORS:-0}
SEC_APP_HEADER=${SEC_APP_HEADER:-0}
SEC_EXIDX=${SEC_EXIDX:-0}

# Calculated values
FLASH_USED=$((SEC_TEXT + SEC_RODATA + SEC_DATA))
FLASH_FREE=$((APP_FLASH_TOTAL - FLASH_USED))
if [ "$FLASH_FREE" -lt 0 ]; then FLASH_FREE=0; fi
FLASH_PCT=$((FLASH_USED * 100 / APP_FLASH_TOTAL))

RAM_STATIC=$((SEC_MSTACK + SEC_PSTACK + SEC_DATA + SEC_BSS))
RAM_COMMITTED=$((RAM_STATIC + SEC_HEAP))
RAM_FREE=$((MCU_RAM_TOTAL - RAM_COMMITTED))
RAM_STATIC_PCT=$((RAM_STATIC * 100 / MCU_RAM_TOTAL))
RAM_HEAP_PCT=$((SEC_HEAP * 100 / MCU_RAM_TOTAL))

# ─── Extract BSS/Data symbols ───────────────────────────────────────────────

# Get all RAM symbols (BSS + data) sorted by size descending
# Strip leading zeros from size field to prevent octal interpretation in sh
NM_OUTPUT=$(arm-none-eabi-nm --print-size --size-sort --reverse-sort --radix=d "$ELF_FILE" 2>/dev/null | awk '$3 == "b" || $3 == "B" || $3 == "d" || $3 == "D" {gsub(/^0+/, "", $2); if ($2 == "") $2 = "0"; print $2, $4}')

# ─── Categorize symbols ─────────────────────────────────────────────────────

categorize_symbol() {
    _sym=$1
    case "$_sym" in
        ch_c0_idle_thread_wa)           echo "ChibiOS Kernel" ;;
        wa_*|*_thread_wa)               echo "Thread Stack" ;;
        SDU[0-9]*)                      echo "USB Serial" ;;
        USBD[0-9]*)                     echo "USB Driver" ;;
        ep[0-9]*state|ep0setup_buffer)  echo "USB Driver" ;;
        vcom_string*|vcom_strings)      echo "USB Driver" ;;
        usb_parser|err_resp_buf)        echo "Attentio Protocol" ;;
        resp_buf|md_payload)            echo "Attentio Protocol" ;;
        evt_buf*|payload*)              echo "Attentio Protocol" ;;
        micb_send_fns|micb_session)     echo "Attentio Protocol" ;;
        cmd_storage|cmd_write_idx)      echo "Attentio Protocol" ;;
        ch0|ch_system|ch_memcore)       echo "ChibiOS Kernel" ;;
        ch_factory|default_heap)        echo "ChibiOS Kernel" ;;
        _pal_events)                    echo "ChibiOS HAL" ;;
        PWMD*|dma|dma_stream|dma_ready) echo "ChibiOS HAL" ;;
        SD[0-9]*)                       echo "Serial Driver" ;;
        sd_*_buf*)                      echo "Serial Driver" ;;
        EFLD*)                          echo "EFL Driver" ;;
        pwm_buf)                        echo "LED Driver" ;;
        serial_cfg|linecoding)          echo "USB Config" ;;
        *)                              echo "Application" ;;
    esac
}

# ═══════════════════════════════════════════════════════════════════════════
# OUTPUT
# ═══════════════════════════════════════════════════════════════════════════

# ─── Header ──────────────────────────────────────────────────────────────────
printf "\n"
printf "%b" "$BOLD_WHITE"
printf "═══════════════════════════════════════════════════════════════════════\n"
printf "  MEMORY REPORT: %s\n" "$(basename "$ELF_FILE")"
printf "  Target: %s (%dKB Flash, %dKB RAM)\n" "$MCU_NAME" $((MCU_FLASH_TOTAL / 1024)) $((MCU_RAM_TOTAL / 1024))
if [ -f "$MAP_FILE" ]; then
    printf "  Map:    %s\n" "$(basename "$MAP_FILE")"
fi
printf "═══════════════════════════════════════════════════════════════════════%b\n" "$RESET"

# ─── 1. Flash Usage ─────────────────────────────────────────────────────────
section_header "FLASH ($((APP_FLASH_TOTAL / 1024))KB available for application)"

printf "  %-16s %8s  %5s   %s\n" "Section" "Bytes" "%" "Bar"
sep

# .text
TEXT_PCT=$((SEC_TEXT * 100 / APP_FLASH_TOTAL))
printf "  %-16s %8s  " ".text" "$(fmt_num $SEC_TEXT)"
pct_color $TEXT_PCT
printf "   "
print_bar $SEC_TEXT $APP_FLASH_TOTAL $BAR_WIDTH
printf "\n"

# .rodata
RODATA_PCT=$((SEC_RODATA * 100 / APP_FLASH_TOTAL))
printf "  %-16s %8s  " ".rodata" "$(fmt_num $SEC_RODATA)"
pct_color $RODATA_PCT
printf "   "
print_bar $SEC_RODATA $APP_FLASH_TOTAL $BAR_WIDTH
printf "\n"

# .data (LMA)
DATA_PCT=$((SEC_DATA * 100 / APP_FLASH_TOTAL))
printf "  %-16s %8s  " ".data (LMA)" "$(fmt_num $SEC_DATA)"
pct_color $DATA_PCT
printf "   "
print_bar $SEC_DATA $APP_FLASH_TOTAL $BAR_WIDTH
printf "\n"

# .vectors
if [ "$SEC_VECTORS" -gt 0 ]; then
    VEC_PCT=$((SEC_VECTORS * 100 / APP_FLASH_TOTAL))
    printf "  %-16s %8s  " ".vectors" "$(fmt_num $SEC_VECTORS)"
    pct_color $VEC_PCT
    printf "   %b(interrupt vector table)%b\n" "$DIM" "$RESET"
fi

sep

# Total
printf "  %b%-16s %8s  " "$BOLD" "TOTAL"  "$(fmt_num $FLASH_USED)"
pct_color $FLASH_PCT
printf "   "
print_bar $FLASH_USED $APP_FLASH_TOTAL $BAR_WIDTH
printf "%b\n" "$RESET"

printf "  %b%-16s %8s  %3d%%%b\n" "$DIM" "Free" "$(fmt_num $FLASH_FREE)" $((100 - FLASH_PCT)) "$RESET"

# ─── Flash memory map (full chip, proportional) ─────────────────────────────
printf "\n  %bFull Chip Flash (128KB = 131,072 bytes):%b\n" "$BOLD" "$RESET"

# Calculate proportional widths for a 64-char wide bar
_fbar=64
_bl_w=$((BOOTLOADER_SIZE * _fbar / MCU_FLASH_TOTAL))
_hdr_w=$((APP_HEADER_SIZE * _fbar / MCU_FLASH_TOTAL))
_app_used_w=$((FLASH_USED * _fbar / MCU_FLASH_TOTAL))
_efl_w=$((EFL_SIZE * _fbar / MCU_FLASH_TOTAL))

# Ensure minimums for visible regions
if [ "$_bl_w" -lt 2 ]; then _bl_w=2; fi
if [ "$_hdr_w" -lt 1 ]; then _hdr_w=1; fi
if [ "$_efl_w" -lt 2 ]; then _efl_w=2; fi
if [ "$_app_used_w" -lt 1 ]; then _app_used_w=1; fi

# App free fills the remainder
_app_free_w=$((_fbar - _bl_w - _hdr_w - _app_used_w - _efl_w))
if [ "$_app_free_w" -lt 0 ]; then _app_free_w=0; fi

printf "  ┌"
_i=0; while [ "$_i" -lt "$_fbar" ]; do printf "─"; _i=$((_i+1)); done
printf "┐\n"

printf "  │"
printf "%b" "$RED"
_i=0; while [ "$_i" -lt "$_bl_w" ]; do printf "▓"; _i=$((_i+1)); done
printf "%b" "$YELLOW"
_i=0; while [ "$_i" -lt "$_hdr_w" ]; do printf "▒"; _i=$((_i+1)); done
printf "%b" "$BOLD_CYAN"
_i=0; while [ "$_i" -lt "$_app_used_w" ]; do printf "█"; _i=$((_i+1)); done
printf "%b" "$DIM"
_i=0; while [ "$_i" -lt "$_app_free_w" ]; do printf "░"; _i=$((_i+1)); done
printf "%b" "$GREEN"
_i=0; while [ "$_i" -lt "$_efl_w" ]; do printf "▓"; _i=$((_i+1)); done
printf "%b│\n" "$RESET"

printf "  └"
_i=0; while [ "$_i" -lt "$_fbar" ]; do printf "─"; _i=$((_i+1)); done
printf "┘\n"

printf "   %b▓%b Boot  %b▒%b Hdr  %b█%b App code  %b░%b App free  %b▓%b EFL\n" \
    "$RED" "$RESET" "$YELLOW" "$RESET" "$BOLD_CYAN" "$RESET" \
    "$DIM" "$RESET" "$GREEN" "$RESET"

printf "\n"
printf "  %-20s %8s  %5s   %s\n" "Region" "Bytes" "%Chip" "Address Range"
sep

_bl_pct=$((BOOTLOADER_SIZE * 100 / MCU_FLASH_TOTAL))
printf "  %-20s %8s  %3d%%   %b0x08000000 - 0x08003FFF%b\n" \
    "Bootloader" "$(fmt_num $BOOTLOADER_SIZE)" "$_bl_pct" "$DIM" "$RESET"

printf "  %-20s %8s   <1%%   %b0x08004000 - 0x080040FF%b\n" \
    "App header + padding" "$(fmt_num $APP_HEADER_SIZE)" "$DIM" "$RESET"

_app_used_pct=$((FLASH_USED * 100 / MCU_FLASH_TOTAL))
printf "  %-20s %8s  %3d%%   %b0x08004100 + ...%b\n" \
    "App code (used)" "$(fmt_num $FLASH_USED)" "$_app_used_pct" "$DIM" "$RESET"

_app_free_pct=$((FLASH_FREE * 100 / MCU_FLASH_TOTAL))
printf "  %b%-20s %8s  %3d%%   (available for growth)%b\n" \
    "$DIM" "App free" "$(fmt_num $FLASH_FREE)" "$_app_free_pct" "$RESET"

_efl_pct=$((EFL_SIZE * 100 / MCU_FLASH_TOTAL))
printf "  %-20s %8s  %3d%%   %b0x0801E000 - 0x0801FFFF%b\n" \
    "EFL storage" "$(fmt_num $EFL_SIZE)" "$_efl_pct" "$DIM" "$RESET"

# ─── 2. RAM Overview ────────────────────────────────────────────────────────
section_header "RAM ($(fmt_num $MCU_RAM_TOTAL) bytes)"

printf "  %-16s %8s  %5s   %s\n" "Section" "Bytes" "%RAM" "Description"
sep

# .mstack
MSTACK_PCT=$((SEC_MSTACK * 100 / MCU_RAM_TOTAL))
printf "  %-16s %8s  " ".mstack" "$(fmt_num $SEC_MSTACK)"
pct_color $MSTACK_PCT
printf "   %bISR / exception stack%b\n" "$DIM" "$RESET"

# .pstack
PSTACK_PCT=$((SEC_PSTACK * 100 / MCU_RAM_TOTAL))
printf "  %-16s %8s  " ".pstack" "$(fmt_num $SEC_PSTACK)"
pct_color $PSTACK_PCT
printf "   %bmain() thread stack%b\n" "$DIM" "$RESET"

# .data
DATA_RAM_PCT=$((SEC_DATA * 100 / MCU_RAM_TOTAL))
printf "  %-16s %8s  " ".data" "$(fmt_num $SEC_DATA)"
pct_color $DATA_RAM_PCT
printf "   %bInitialized globals%b\n" "$DIM" "$RESET"

# .bss
BSS_PCT=$((SEC_BSS * 100 / MCU_RAM_TOTAL))
printf "  %-16s %8s  " ".bss" "$(fmt_num $SEC_BSS)"
pct_color $BSS_PCT
printf "   %bZero-init globals (thread stacks, buffers)%b\n" "$DIM" "$RESET"

sep

printf "  %b%-16s %8s  " "$BOLD" "Fixed total" "$(fmt_num $RAM_STATIC)"
pct_color $RAM_STATIC_PCT
printf "%b  %bLinker-placed (stacks + globals)%b\n" "$RESET" "$DIM" "$RESET"

printf "  %-16s %8s  " ".heap" "$(fmt_num $SEC_HEAP)"
pct_color $RAM_HEAP_PCT
printf "   %bDynamic region (ChibiOS core allocator)%b\n" "$DIM" "$RESET"

sep

printf "  %b%-16s %8s  " "$BOLD" "Mapped total" "$(fmt_num $RAM_COMMITTED)"
COMMITTED_PCT=$((RAM_COMMITTED * 100 / MCU_RAM_TOTAL))
pct_color $COMMITTED_PCT
printf "%b" "$RESET"
if [ "$RAM_FREE" -gt 0 ]; then
    printf "   %b(%d bytes alignment padding)%b" "$DIM" "$RAM_FREE" "$RESET"
fi
printf "\n"
printf "\n  %bNote:%b Fixed = space reserved at link time (stacks, globals, static\n" "$BOLD" "$RESET"
printf "  buffers). Heap = leftover RAM given to ChibiOS core allocator.\n"
printf "  Currently %bno code allocates from the heap at runtime%b — all memory\n" "$BOLD" "$RESET"
printf "  is static. New fixed allocations shrink the heap 1:1, but since the\n"
printf "  heap is idle, the effective free budget for new statics is ~%s bytes.\n" "$(fmt_num $SEC_HEAP)"

# RAM visual map
printf "\n  %bRAM Map (24KB = 24,576 bytes):%b\n" "$DIM" "$RESET"

# Calculate proportional widths for a 64-char wide bar
_bar_total=64
_mstack_w=$((SEC_MSTACK * _bar_total / MCU_RAM_TOTAL))
_pstack_w=$((SEC_PSTACK * _bar_total / MCU_RAM_TOTAL))
_data_w=$((SEC_DATA * _bar_total / MCU_RAM_TOTAL))
_bss_w=$((SEC_BSS * _bar_total / MCU_RAM_TOTAL))
_heap_w=$((SEC_HEAP * _bar_total / MCU_RAM_TOTAL))

# Ensure minimum 1 char for non-zero sections, handle .data specially
if [ "$SEC_DATA" -gt 0 ] && [ "$_data_w" -eq 0 ]; then _data_w=1; fi
if [ "$_mstack_w" -eq 0 ]; then _mstack_w=1; fi
if [ "$_pstack_w" -eq 0 ]; then _pstack_w=1; fi

# Adjust heap to fill remainder
_used_w=$((_mstack_w + _pstack_w + _data_w + _bss_w))
_heap_w=$((_bar_total - _used_w))

printf "  ┌"
_i=0; while [ "$_i" -lt "$_bar_total" ]; do printf "─"; _i=$((_i+1)); done
printf "┐\n"

# Colored segments
printf "  │"
printf "%b" "$BOLD_RED"
_i=0; while [ "$_i" -lt "$_mstack_w" ]; do printf "▓"; _i=$((_i+1)); done
printf "%b" "$BOLD_YELLOW"
_i=0; while [ "$_i" -lt "$_pstack_w" ]; do printf "▓"; _i=$((_i+1)); done
printf "%b" "$BOLD_GREEN"
_i=0; while [ "$_i" -lt "$_data_w" ]; do printf "▒"; _i=$((_i+1)); done
printf "%b" "$BOLD_CYAN"
_i=0; while [ "$_i" -lt "$_bss_w" ]; do printf "█"; _i=$((_i+1)); done
printf "%b" "$DIM"
_i=0; while [ "$_i" -lt "$_heap_w" ]; do printf "░"; _i=$((_i+1)); done
printf "%b│\n" "$RESET"

printf "  └"
_i=0; while [ "$_i" -lt "$_bar_total" ]; do printf "─"; _i=$((_i+1)); done
printf "┘\n"

printf "   %b▓%b MSP  %b▓%b PSP  %b▒%b .data  %b█%b .bss  %b░%b heap (unused)\n" \
    "$BOLD_RED" "$RESET" "$BOLD_YELLOW" "$RESET" "$BOLD_GREEN" "$RESET" \
    "$BOLD_CYAN" "$RESET" "$DIM" "$RESET"

# ─── 3. BSS Breakdown ───────────────────────────────────────────────────────
section_header "BSS BREAKDOWN ($(fmt_num $SEC_BSS) bytes) — Top RAM Consumers"

# Parse symbols and build categorized output
# We use a temp file because POSIX sh doesn't have arrays
TMPFILE=$(mktemp)
trap 'rm -f "$TMPFILE"' EXIT

echo "$NM_OUTPUT" | while IFS=' ' read -r size name; do
    if [ -n "$size" ] && [ -n "$name" ]; then
        cat=$(categorize_symbol "$name")
        echo "$size $name $cat"
    fi
done > "$TMPFILE"

# Print top symbols
printf "  %-30s %8s  %5s  %s\n" "Symbol" "Bytes" "%BSS" "Category"
sep

# Count total for "others"
_shown_total=0
_line_count=0
_total_symbols=0

# Count total symbols
_total_symbols=$(wc -l < "$TMPFILE")

# Show top 20 symbols
head -20 "$TMPFILE" | while IFS=' ' read -r size name cat1 cat2; do
    cat="$cat1"
    if [ -n "$cat2" ]; then cat="$cat1 $cat2"; fi
    if [ "$SEC_BSS" -gt 0 ]; then
        _spct=$((size * 1000 / SEC_BSS))
        _swhole=$((_spct / 10))
        _sfrac=$((_spct % 10))
    else
        _swhole=0
        _sfrac=0
    fi

    # Color by category
    case "$cat" in
        "Thread Stack")   _ccolor="$CYAN" ;;
        "USB Serial")     _ccolor="$YELLOW" ;;
        "USB Driver")     _ccolor="$YELLOW" ;;
        "Attentio Protocol")    _ccolor="$GREEN" ;;
        "ChibiOS Kernel") _ccolor="$RED" ;;
        "ChibiOS HAL")    _ccolor="$RED" ;;
        *)                _ccolor="$WHITE" ;;
    esac

    printf "  %-30s %8s  %2d.%d%%  %b%-15s%b\n" \
        "$name" "$(fmt_num "$size")" "$_swhole" "$_sfrac" "$_ccolor" "$cat" "$RESET"
done

# Calculate and show "others"
_top20_total=$(head -20 "$TMPFILE" | awk '{s+=$1} END {print s+0}')
_others_count=$((_total_symbols - 20))
if [ "$_others_count" -lt 0 ]; then _others_count=0; fi
_others_total=$((SEC_BSS + SEC_DATA - _top20_total))
if [ "$_others_total" -lt 0 ]; then _others_total=0; fi

if [ "$_others_count" -gt 0 ] && [ "$_others_total" -gt 0 ]; then
    if [ "$SEC_BSS" -gt 0 ]; then
        _opct=$((_others_total * 1000 / SEC_BSS))
        _owhole=$((_opct / 10))
        _ofrac=$((_opct % 10))
    else
        _owhole=0
        _ofrac=0
    fi
    printf "  %b%-30s %8s  %2d.%d%%  (%d symbols)%b\n" \
        "$DIM" "(others)" "$(fmt_num "$_others_total")" "$_owhole" "$_ofrac" "$_others_count" "$RESET"
fi

# Category summary
printf "\n  %bBy Category:%b\n" "$BOLD" "$RESET"
sep

# Aggregate by category
_cat_tmp=$(mktemp)
trap 'rm -f "$TMPFILE" "$_cat_tmp"' EXIT

awk '{
    # Reconstruct category (may be two words)
    cat = ""
    for (i = 3; i <= NF; i++) {
        if (cat != "") cat = cat " "
        cat = cat $i
    }
    sums[cat] += $1
    counts[cat]++
}
END {
    for (cat in sums) {
        printf "%d %d %s\n", sums[cat], counts[cat], cat
    }
}' "$TMPFILE" | sort -rn > "$_cat_tmp"

while IFS=' ' read -r _csize _ccount _ccat1 _ccat2; do
    _ccat="$_ccat1"
    if [ -n "$_ccat2" ]; then _ccat="$_ccat1 $_ccat2"; fi

    if [ "$SEC_BSS" -gt 0 ]; then
        _cpct=$((_csize * 1000 / SEC_BSS))
        _cwhole=$((_cpct / 10))
        _cfrac=$((_cpct % 10))
    else
        _cwhole=0
        _cfrac=0
    fi

    case "$_ccat" in
        "Thread Stack")   _ccolor="$CYAN" ;;
        "USB Serial")     _ccolor="$YELLOW" ;;
        "USB Driver")     _ccolor="$YELLOW" ;;
        "Attentio Protocol")    _ccolor="$GREEN" ;;
        "ChibiOS Kernel") _ccolor="$RED" ;;
        "ChibiOS HAL")    _ccolor="$RED" ;;
        "Serial Driver")  _ccolor="$YELLOW" ;;
        "EFL Driver")     _ccolor="$BLUE" ;;
        "LED Driver")     _ccolor="$BLUE" ;;
        "USB Config")     _ccolor="$YELLOW" ;;
        *)                _ccolor="$WHITE" ;;
    esac

    printf "  %b%-20s%b  %8s  %2d.%d%%  " "$_ccolor" "$_ccat" "$RESET" "$(fmt_num "$_csize")" "$_cwhole" "$_cfrac"
    print_bar "$_csize" "$SEC_BSS" 25
    printf "  %b(%d symbols)%b\n" "$DIM" "$_ccount" "$RESET"
done < "$_cat_tmp"

rm -f "$_cat_tmp"

# ─── 4. Thread Stack Analysis ───────────────────────────────────────────────
section_header "THREAD STACKS"

printf "  All threads include %b+%d bytes%b for LOG_* printf buffer (always compiled in).\n" "$BOLD" 512 "$RESET"
printf "  ChibiOS thread_t overhead: ~%d bytes per thread (context + TCB).\n\n" "$THREAD_OVERHEAD"

printf "  %-20s %6s  %8s  %9s  %s\n" "Thread" "WA" "Overhead" "Usable" "Status"
sep

# Extract thread WA symbols from nm output to get actual sizes
# (These include the thread_t overhead that ChibiOS adds)
print_thread_row() {
    _tname=$1
    _tsym=$2
    _tbase=$3          # Base stack size (without _RT_DEBUG_EXTRA)
    _tdebug_extra=512  # _RT_DEBUG_EXTRA

    # Get actual WA size from nm
    _actual_wa=$(echo "$NM_OUTPUT" | awk -v sym="$_tsym" '$2 == sym {print $1; exit}')
    if [ -z "$_actual_wa" ]; then
        _actual_wa=$((_tbase + _tdebug_extra + THREAD_OVERHEAD))
    fi

    _declared=$((_tbase + _tdebug_extra))
    _usable=$((_actual_wa - THREAD_OVERHEAD))
    _stack_for_code=$((_usable - LOG_BUF_SIZE))

    # Determine status
    # Thresholds: <256 DANGER, <400 TIGHT (one large stack frame can overflow),
    #             <600 CAUTION (LTO risk), >=600 OK
    if [ "$_stack_for_code" -le 256 ]; then
        _status="${BOLD_RED}DANGER${RESET}  ${DIM}only ${_stack_for_code}B for call frames${RESET}"
    elif [ "$_stack_for_code" -le 400 ]; then
        _status="${BOLD_YELLOW}TIGHT${RESET}   ${DIM}${_stack_for_code}B for call frames${RESET}"
    elif [ "$_stack_for_code" -le 600 ]; then
        _status="${YELLOW}CAUTION${RESET} ${DIM}${_stack_for_code}B for call frames${RESET}"
    else
        _status="${GREEN}OK${RESET}      ${DIM}${_stack_for_code}B for call frames${RESET}"
    fi

    printf "  %-20s %6s  %8s  %9s  %b\n" \
        "$_tname" "$(fmt_num "$_actual_wa")" "~$THREAD_OVERHEAD" "$(fmt_num "$_usable")" "$_status"
}

print_thread_row "button_driver"   "wa_button_thread"       256
print_thread_row "state_machine"   "wa_sm_thread"           512
print_thread_row "animation"       "wa_anim_thread"         512
print_thread_row "usb_adapter"     "wa_usb_adapter_thread"  256

# System stacks (no thread_t overhead, no LOG usage)
printf "\n"
printf "  %b%-20s %6s  %8s  %9s  %s%b\n" "$DIM" \
    "main (PSP)" "$(fmt_num $SEC_PSTACK)" "--" "$(fmt_num $SEC_PSTACK)" "Linker-defined process stack" "$RESET"
printf "  %b%-20s %6s  %8s  %9s  %s%b\n" "$DIM" \
    "ISR (MSP)" "$(fmt_num $SEC_MSTACK)" "--" "$(fmt_num $SEC_MSTACK)" "Linker-defined exception stack" "$RESET"

# ChibiOS idle thread
_idle_wa=$(echo "$NM_OUTPUT" | awk '$2 == "ch_c0_idle_thread_wa" {print $1; exit}')
_idle_wa=${_idle_wa:-152}
printf "  %b%-20s %6s  %8s  %9s  %s%b\n" "$DIM" \
    "idle (ChibiOS)" "$(fmt_num "$_idle_wa")" "~$THREAD_OVERHEAD" "--" "System idle thread" "$RESET"

printf "\n  %bStack budget per thread:%b\n" "$BOLD" "$RESET"
printf "  ┌──────────────────────────────────────────────────────────────┐\n"
printf "  │ %bWA total%b = thread_t (~%d) + LOG buffer (%d) + user stack   │\n" "$BOLD" "$RESET" "$THREAD_OVERHEAD" "$LOG_BUF_SIZE"
printf "  │                                                              │\n"
printf "  │ The %buser stack%b is what remains for actual function call      │\n" "$BOLD" "$RESET"
printf "  │ frames. With LTO, inlined functions share the caller's       │\n"
printf "  │ frame — use %b__attribute__((noinline))%b on large functions.    │\n" "$BOLD_YELLOW" "$RESET"
printf "  └──────────────────────────────────────────────────────────────┘\n"

printf "\n  %bRuntime validation:%b  Build with 'make debug' and connect via\n" "$BOLD" "$RESET"
printf "  CDC0 serial to see live diagnostics every ~1 s (flags in debug_config.h):\n"
printf "    %bAPP_STACK_WATERMARK%b — thread stack peak-usage watermarks\n" "$BOLD" "$RESET"
printf "    %bAPP_HEAP_ANALYSIS%b   — heap free memory, fragmentation, integrity\n" "$BOLD" "$RESET"
printf "  Both default to %bAPP_DEBUG_BUILD_PLUS_PLUS_ACTIVE%b (1 in debug builds, 0 in\n" "$BOLD" "$RESET"
printf "  release) and are independent of the runtime log level.\n"
printf "  Compare runtime values against the static analysis above.\n"

# ─── 5. RAM Pressure Summary ────────────────────────────────────────────────
section_header "RAM PRESSURE SUMMARY"

# Calculate category totals for the summary
# Thread stacks (app threads only, not idle)
_thread_total=0
for _sym in wa_button_thread wa_sm_thread wa_anim_thread wa_usb_adapter_thread; do
    _sz=$(echo "$NM_OUTPUT" | awk -v s="$_sym" '$2 == s {print $1; exit}')
    _sz=${_sz:-0}
    _thread_total=$((_thread_total + _sz))
done

# USB CDC (SDU1 + SDU2)
_usb_cdc_total=0
for _sym in SDU1 SDU2; do
    _sz=$(echo "$NM_OUTPUT" | awk -v s="$_sym" '$2 == s {print $1; exit}')
    _sz=${_sz:-0}
    _usb_cdc_total=$((_usb_cdc_total + _sz))
done

# USB HAL (USBD1, endpoint states, vcom)
_usb_hal_total=0
for _sym in USBD1 ep0_state ep0setup_buffer ep1instate ep2instate ep2outstate ep3instate ep4instate ep4outstate vcom_string3 vcom_strings; do
    _sz=$(echo "$NM_OUTPUT" | awk -v s="$_sym" '$2 == s {print $1; exit}')
    _sz=${_sz:-0}
    _usb_hal_total=$((_usb_hal_total + _sz))
done

# Attentio Protocol buffers
_ap_total=0
for _sym in usb_parser err_resp_buf resp_buf md_payload micb_send_fns micb_session cmd_storage cmd_write_idx; do
    _sz=$(echo "$NM_OUTPUT" | awk -v s="$_sym" '$2 == s {print $1; exit}')
    _sz=${_sz:-0}
    _ap_total=$((_ap_total + _sz))
done
# Also add evt_buf.* and payload.* variants
_ap_extra=$(echo "$NM_OUTPUT" | awk '$2 ~ /^(evt_buf|payload)\.[0-9]/ {s+=$1} END {print s+0}')
_ap_total=$((_ap_total + _ap_extra))

# ChibiOS kernel (including idle thread)
_kernel_total=0
for _sym in ch0 ch_system ch_memcore ch_factory default_heap ch_c0_idle_thread_wa; do
    _sz=$(echo "$NM_OUTPUT" | awk -v s="$_sym" '$2 == s {print $1; exit}')
    _sz=${_sz:-0}
    _kernel_total=$((_kernel_total + _sz))
done

# ChibiOS HAL
_hal_total=0
for _sym in _pal_events PWMD1 dma dma_stream dma_ready; do
    _sz=$(echo "$NM_OUTPUT" | awk -v s="$_sym" '$2 == s {print $1; exit}')
    _sz=${_sz:-0}
    _hal_total=$((_hal_total + _sz))
done

# Serial driver
_serial_total=0
for _sym in SD2 sd_out_buf2 sd_in_buf2 serial_cfg linecoding; do
    _sz=$(echo "$NM_OUTPUT" | awk -v s="$_sym" '$2 == s {print $1; exit}')
    _sz=${_sz:-0}
    _serial_total=$((_serial_total + _sz))
done

# System stacks (from linker sections, not BSS)
_sys_stacks=$((SEC_MSTACK + SEC_PSTACK))

# Everything else
_categorized=$((_thread_total + _usb_cdc_total + _usb_hal_total + _ap_total + _kernel_total + _hal_total + _serial_total))
_app_other=$((SEC_BSS + SEC_DATA - _categorized))
if [ "$_app_other" -lt 0 ]; then _app_other=0; fi

printf "  %-32s %8s  %5s\n" "Category" "Bytes" "%RAM"
sep

print_pressure_row() {
    _plabel=$1
    _psize=$2
    _pnote=$3
    _ppct=$((_psize * 1000 / MCU_RAM_TOTAL))
    _pwhole=$((_ppct / 10))
    _pfrac=$((_ppct % 10))
    printf "  %-32s %8s  %2d.%d%%  " "$_plabel" "$(fmt_num "$_psize")" "$_pwhole" "$_pfrac"
    print_bar "$_psize" "$MCU_RAM_TOTAL" 20
    if [ -n "$_pnote" ]; then
        printf "  %b%s%b" "$DIM" "$_pnote" "$RESET"
    fi
    printf "\n"
}

print_pressure_row "System stacks (MSP + PSP)" "$_sys_stacks" ""
print_pressure_row "Thread working areas (4)" "$_thread_total" ""
print_pressure_row "USB CDC drivers (SDU1+SDU2)" "$_usb_cdc_total" ""
print_pressure_row "USB HAL + descriptors" "$_usb_hal_total" ""
print_pressure_row "Attentio Protocol buffers" "$_ap_total" ""
print_pressure_row "ChibiOS kernel" "$_kernel_total" ""
print_pressure_row "ChibiOS HAL (PAL, PWM, DMA)" "$_hal_total" ""
print_pressure_row "Serial driver (USART)" "$_serial_total" ""
print_pressure_row "Application data" "$_app_other" ""

sep

_static_pct=$((RAM_STATIC * 1000 / MCU_RAM_TOTAL))
_swhole=$((_static_pct / 10))
_sfrac=$((_static_pct % 10))
printf "  %b%-32s %8s  %2d.%d%%%b\n" "$BOLD" "TOTAL FIXED" "$(fmt_num "$RAM_STATIC")" "$_swhole" "$_sfrac" "$RESET"

_heap_pct=$((SEC_HEAP * 1000 / MCU_RAM_TOTAL))
_hwhole=$((_heap_pct / 10))
_hfrac=$((_heap_pct % 10))
printf "  %b%-32s %8s  %2d.%d%%  (idle — no runtime allocations)%b\n" "$DIM" "Heap (dynamic region)" "$(fmt_num "$SEC_HEAP")" "$_hwhole" "$_hfrac" "$RESET"

_committed_pct=$((RAM_COMMITTED * 1000 / MCU_RAM_TOTAL))
_cwhole=$((_committed_pct / 10))
_cfrac=$((_committed_pct % 10))
printf "  %b%-32s %8s  %2d.%d%%%b\n" "$BOLD" "MAPPED (fixed + heap)" "$(fmt_num "$RAM_COMMITTED")" "$_cwhole" "$_cfrac" "$RESET"

if [ "$RAM_FREE" -gt 0 ]; then
    printf "  %b%-32s %8s  (alignment padding)%b\n" "$DIM" "Padding" "$(fmt_num "$RAM_FREE")" "$RESET"
fi

# Grand bar
printf "\n  %bRAM budget:%b  " "$BOLD" "$RESET"
print_bar "$RAM_STATIC" "$MCU_RAM_TOTAL" 50
printf "\n"
printf "               %b" "$DIM"
printf "Fixed (%d%%)%b" "$_swhole" "$RESET"
_spaces=$((50 - 14))
_i=0; while [ "$_i" -lt "$_spaces" ]; do printf " "; _i=$((_i+1)); done
printf "%bHeap (idle)%b\n" "$DIM" "$RESET"

# ─── 6. Warnings & Recommendations ──────────────────────────────────────────
section_header "WARNINGS & RECOMMENDATIONS"

_warn_count=0

# Check USB adapter thread
_usb_wa=$(echo "$NM_OUTPUT" | awk '$2 == "wa_usb_adapter_thread" {print $1; exit}')
_usb_wa=${_usb_wa:-984}
_usb_usable=$((_usb_wa - THREAD_OVERHEAD))
_usb_for_code=$((_usb_usable - LOG_BUF_SIZE))

if [ "$_usb_for_code" -le 600 ]; then
    printf "  %b[!]%b USB adapter thread: WA=%d, overhead~%d, LOG buffer=%d\n" \
        "$BOLD_YELLOW" "$RESET" "$_usb_wa" "$THREAD_OVERHEAD" "$LOG_BUF_SIZE"
    printf "      → only %b%d bytes%b remain for function call frames.\n" \
        "$BOLD" "$_usb_for_code" "$RESET"
    printf "      Increasing RT_USB_ADAPTER_THREAD_WA_SIZE base from 256 to 384\n"
    printf "      costs 128 bytes of heap (which is idle, so effectively free).\n"
    printf "      %bNew functions in micb.c MUST use __attribute__((noinline)).%b\n\n" "$BOLD_YELLOW" "$RESET"
    _warn_count=$((_warn_count + 1))
fi

# Check button thread
_btn_wa=$(echo "$NM_OUTPUT" | awk '$2 == "wa_button_thread" {print $1; exit}')
_btn_wa=${_btn_wa:-984}
_btn_usable=$((_btn_wa - THREAD_OVERHEAD))
_btn_for_code=$((_btn_usable - LOG_BUF_SIZE))

if [ "$_btn_for_code" -le 600 ]; then
    printf "  %b[!]%b Button thread: WA=%d, overhead~%d, LOG buffer=%d\n" \
        "$BOLD_YELLOW" "$RESET" "$_btn_wa" "$THREAD_OVERHEAD" "$LOG_BUF_SIZE"
    printf "      → only %b%d bytes%b remain for function call frames.\n" \
        "$BOLD" "$_btn_for_code" "$RESET"
    printf "      Increasing BTN_THREAD_WA_SIZE base from 256 to 384\n"
    printf "      costs 128 bytes of heap (idle, effectively free).\n\n"
    _warn_count=$((_warn_count + 1))
fi

# Check flash usage
if [ "$FLASH_PCT" -ge 85 ]; then
    printf "  %b[!]%b Flash is %b%d%%%b full (%s of %s bytes).\n" \
        "$BOLD_YELLOW" "$RESET" "$BOLD" "$FLASH_PCT" "$RESET" \
        "$(fmt_num $FLASH_USED)" "$(fmt_num $APP_FLASH_TOTAL)"
    printf "      Consider -Os optimization or reviewing .rodata usage.\n\n"
    _warn_count=$((_warn_count + 1))
fi

# LTO reminder (always show)
printf "  %b[i]%b LTO is enabled (USE_LTO=yes). Functions added to micb.c command\n" "$BOLD_CYAN" "$RESET"
printf "      handlers should use %b__attribute__((noinline))%b to prevent their\n" "$BOLD" "$RESET"
printf "      stack frames from being merged into the USB adapter thread.\n\n"

# Heap note
printf "  %b[i]%b The heap (%s bytes) is managed by ChibiOS (CH_CFG_USE_HEAP=TRUE)\n" "$BOLD_CYAN" "$RESET" "$(fmt_num $SEC_HEAP)"
printf "      but nothing allocates from it at runtime. It acts as a buffer:\n"
printf "      adding fixed allocations (new statics, larger stacks) shrinks\n"
printf "      it 1:1, which is harmless as long as the heap stays unused.\n"
printf "      Runtime monitoring: APP_HEAP_ANALYSIS in debug_config.h\n"
printf "      (defaults to APP_DEBUG_BUILD_PLUS_PLUS_ACTIVE) prints heap status over CDC0.\n\n"

if [ "$_warn_count" -eq 0 ]; then
    printf "  %b✓  No critical warnings.%b\n\n" "$BOLD_GREEN" "$RESET"
fi

# ─── Footer ─────────────────────────────────────────────────────────────────
printf "%b═══════════════════════════════════════════════════════════════════════%b\n\n" "$DIM" "$RESET"
