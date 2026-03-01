#!/usr/bin/env bash
#
# Test script for st-flash workarounds on STM32C0
#
# Problem: st-flash appears to erase the bootloader (0x08000000) when 
#          writing to application region (0x08004000)
#
# This script tests various st-flash options to find a working configuration.
#
# Prerequisites:
#   - Bootloader flashed and working (shows "EngEmil.io Bootloader DFU Mode" in lsusb)
#   - Application binary built (fw_al1mb1_signed.bin)
#
# Usage:
#   ./test_stflash_workarounds.sh [test_number]
#
#   Run without arguments to see available tests.
#   Run with test number to execute that specific test.
#

set -euo pipefail

# Configuration
APP_BIN="../fw_al1mb1/build/fw_al1mb1_signed.bin"
APP_ADDR="0x08004000"
BOOTLOADER_ADDR="0x08000000"
FLASH_SIZE="128k"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

info() { echo -e "${GREEN}[INFO]${NC} $*"; }
warn() { echo -e "${YELLOW}[WARN]${NC} $*"; }
error() { echo -e "${RED}[ERROR]${NC} $*"; }

check_bootloader() {
    info "Checking if custom bootloader is present..."
    if lsusb | grep -q "EngEmil.io Bootloader"; then
        info "Custom bootloader detected (EngEmil.io Bootloader DFU Mode)"
        return 0
    elif lsusb | grep -q "0483:df11.*STMicroelectronics"; then
        error "STM32 ROM bootloader detected - custom bootloader was erased!"
        return 1
    else
        warn "No DFU device detected - device may be running application or disconnected"
        return 2
    fi
}

check_prerequisites() {
    if [ ! -f "$APP_BIN" ]; then
        error "Application binary not found: $APP_BIN"
        error "Build the application first: cd fw_al1mb1 && make"
        exit 1
    fi
    
    if ! command -v st-flash &> /dev/null; then
        error "st-flash not found. Install stlink tools."
        exit 1
    fi
    
    info "st-flash version: $(st-flash --version)"
    info "Application binary: $APP_BIN ($(stat -c%s "$APP_BIN") bytes)"
}

flash_bootloader() {
    info "Re-flashing bootloader to restore device..."
    local bl_bin="../bootloader/bootloader/build/bootloader.bin"
    if [ ! -f "$bl_bin" ]; then
        error "Bootloader binary not found: $bl_bin"
        error "Build the bootloader first: cd bootloader/bootloader && make"
        return 1
    fi
    st-flash erase
    st-flash --reset write "$bl_bin" "$BOOTLOADER_ADDR"
    sleep 2
    check_bootloader
}

# Test functions - each tests a different st-flash configuration
test_baseline() {
    info "TEST: Baseline (current broken behavior)"
    info "Command: st-flash --reset write $APP_BIN $APP_ADDR"
    st-flash --reset write "$APP_BIN" "$APP_ADDR"
    sleep 2
    check_bootloader
}

test_no_reset() {
    info "TEST: Without --reset flag"
    info "Command: st-flash write $APP_BIN $APP_ADDR"
    st-flash write "$APP_BIN" "$APP_ADDR"
    sleep 2
    check_bootloader
}

test_hot_plug() {
    info "TEST: With --hot-plug (connect without reset)"
    info "Command: st-flash --hot-plug --reset write $APP_BIN $APP_ADDR"
    st-flash --hot-plug --reset write "$APP_BIN" "$APP_ADDR"
    sleep 2
    check_bootloader
}

test_flash_size() {
    info "TEST: Explicit flash size (--flash $FLASH_SIZE)"
    info "Command: st-flash --flash $FLASH_SIZE --reset write $APP_BIN $APP_ADDR"
    st-flash --flash "$FLASH_SIZE" --reset write "$APP_BIN" "$APP_ADDR"
    sleep 2
    check_bootloader
}

test_connect_under_reset() {
    info "TEST: With --connect-under-reset"
    info "Command: st-flash --connect-under-reset --reset write $APP_BIN $APP_ADDR"
    st-flash --connect-under-reset --reset write "$APP_BIN" "$APP_ADDR"
    sleep 2
    check_bootloader
}

test_combined_hot_plug_flash_size() {
    info "TEST: Combined --hot-plug and --flash $FLASH_SIZE"
    info "Command: st-flash --hot-plug --flash $FLASH_SIZE --reset write $APP_BIN $APP_ADDR"
    st-flash --hot-plug --flash "$FLASH_SIZE" --reset write "$APP_BIN" "$APP_ADDR"
    sleep 2
    check_bootloader
}

test_no_reset_with_flash_size() {
    info "TEST: No reset with explicit flash size"
    info "Command: st-flash --flash $FLASH_SIZE write $APP_BIN $APP_ADDR"
    st-flash --flash "$FLASH_SIZE" write "$APP_BIN" "$APP_ADDR"
    sleep 2
    check_bootloader
}

test_debug_mode() {
    info "TEST: Debug mode to see what's happening"
    info "Command: st-flash --debug --flash $FLASH_SIZE write $APP_BIN $APP_ADDR"
    st-flash --debug --flash "$FLASH_SIZE" write "$APP_BIN" "$APP_ADDR" 2>&1 | tee /tmp/stflash_debug.log
    sleep 2
    info "Debug output saved to /tmp/stflash_debug.log"
    check_bootloader
}

show_usage() {
    echo "Usage: $0 [test_number|restore]"
    echo ""
    echo "Available tests:"
    echo "  1 - Baseline (current broken behavior)"
    echo "  2 - Without --reset flag"
    echo "  3 - With --hot-plug"
    echo "  4 - Explicit --flash $FLASH_SIZE"
    echo "  5 - With --connect-under-reset"
    echo "  6 - Combined --hot-plug and --flash $FLASH_SIZE"
    echo "  7 - No reset with explicit flash size"
    echo "  8 - Debug mode (verbose output)"
    echo ""
    echo "  restore - Re-flash bootloader to restore device"
    echo ""
    echo "Recommended workflow:"
    echo "  1. Ensure bootloader is working: lsusb | grep EngEmil"
    echo "  2. Run a test: $0 2"
    echo "  3. If bootloader was erased, restore: $0 restore"
    echo "  4. Try next test"
}

main() {
    check_prerequisites
    
    case "${1:-}" in
        1) test_baseline ;;
        2) test_no_reset ;;
        3) test_hot_plug ;;
        4) test_flash_size ;;
        5) test_connect_under_reset ;;
        6) test_combined_hot_plug_flash_size ;;
        7) test_no_reset_with_flash_size ;;
        8) test_debug_mode ;;
        restore) flash_bootloader ;;
        *)
            show_usage
            echo ""
            echo "Current bootloader status:"
            check_bootloader || true
            ;;
    esac
}

main "$@"
