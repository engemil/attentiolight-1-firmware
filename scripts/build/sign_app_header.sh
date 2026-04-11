
#!/usr/bin/env bash
#
# MIT License
# 
# Copyright (c) 2026 EngEmil
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
#
# Sign Application Binary with Size and CRC32
#
# This script is used to sign the application binary with a header containing:
# - Magic number (0xDEADBEEF)
# - Version (4 bytes)
# - Firmware size (4 bytes, signed at offset 8)
# - CRC32 checksum (4 bytes, signed at offset 12)
# - Reserved (16 bytes)
# 
#
# This script:
# 1. Reads the application binary
# 2. Calculates the firmware size (from vector table to end)
# 3. Calculates CRC32 of the firmware (from vector table to end)
# 4. Signs the header with the size and CRC32 at offset 8 and 12
# 5. Writes the signed binary
#
# Dependencies:
#   - bash (4.0+)
#   - dd       (coreutils)
#   - od       (coreutils)
#   - stat     (coreutils)
#   - printf   (coreutils / bash builtin)
#   - gzip     (gzip)
#   - tail     (coreutils)
#   - mktemp   (coreutils)
#
# All dependencies are standard on any Linux distribution (including
# minimal/embedded environments and Docker containers). No additional
# packages are required beyond the base coreutils and gzip.
#
# Application header structure (32 bytes at offset 0):
#   Offset 0:  magic (0xDEADBEEF) - 4 bytes (little-endian)
#   Offset 4:  version            - 4 bytes (little-endian)
#   Offset 8:  size               - 4 bytes (little-endian, SIGNED)
#   Offset 12: crc32              - 4 bytes (little-endian, SIGNED)
#   Offset 16: reserved[4]        - 16 bytes
#
# IMPORTANT: CRC is calculated over firmware starting at offset 0x100
# (vector table), NOT from offset 0x20 (after header).

set -euo pipefail

HEADER_SIZE=32
VECTOR_TABLE_OFFSET=256  # 0x100
MAGIC="DEADBEEF"

# --- Helper functions ---

# Print an error message and exit
die() {
    echo "Error: $*" >&2
    exit 1
}

# Read a 32-bit little-endian value from a file at a given offset.
# Usage: read_le32_hex <file> <byte_offset>
# Outputs the value as an uppercase hex string (no 0x prefix), e.g. "DEADBEEF".
read_le32_hex() {
    local file="$1"
    local offset="$2"
    local bytes

    # Read 4 bytes as individual hex values
    bytes=$(od -An -tx1 -N4 -j"${offset}" "${file}" | tr -d ' \n')

    if [ ${#bytes} -ne 8 ]; then
        die "Failed to read 4 bytes at offset ${offset} from ${file}"
    fi

    # Reconstruct little-endian: bytes are b0 b1 b2 b3 -> value = b3b2b1b0
    local b0="${bytes:0:2}"
    local b1="${bytes:2:2}"
    local b2="${bytes:4:2}"
    local b3="${bytes:6:2}"

    echo "${b3}${b2}${b1}${b0}" | tr 'a-f' 'A-F'
}

# Read a 32-bit little-endian unsigned integer from a file at a given offset.
# Usage: read_le32 <file> <byte_offset>
# Outputs the value as a decimal integer.
# Note: Values above 0x7FFFFFFF (e.g. 0xDEADBEEF) will overflow bash's
# signed arithmetic. Use read_le32_hex for such values.
read_le32() {
    local hex
    hex=$(read_le32_hex "$1" "$2")
    printf '%d' "0x${hex}"
}

# Write a 32-bit little-endian unsigned integer to a file at a given offset.
# Usage: write_le32 <file> <byte_offset> <value>
write_le32() {
    local file="$1"
    local offset="$2"
    local value="$3"

    # Convert value to 4 little-endian bytes
    local b0 b1 b2 b3
    b0=$(( value & 0xFF ))
    b1=$(( (value >> 8) & 0xFF ))
    b2=$(( (value >> 16) & 0xFF ))
    b3=$(( (value >> 24) & 0xFF ))

    # Write using printf + dd (conv=notrunc to overwrite in place)
    printf "\\x$(printf '%02x' ${b0})\\x$(printf '%02x' ${b1})\\x$(printf '%02x' ${b2})\\x$(printf '%02x' ${b3})" \
        | dd of="${file}" bs=1 seek="${offset}" conv=notrunc status=none
}

# Calculate CRC32 of data from a file starting at a given offset to end.
# Uses the gzip trailer which contains a standard CRC32 (same as zlib.crc32).
# Usage: calculate_crc32 <file> <byte_offset>
# Outputs the CRC32 as a decimal integer.
calculate_crc32() {
    local file="$1"
    local offset="$2"
    local hex_crc

    # Extract firmware data from offset to end, compress with gzip,
    # read CRC32 from the gzip trailer (last 8 bytes: 4-byte CRC + 4-byte size).
    hex_crc=$(dd if="${file}" bs=1 skip="${offset}" status=none \
        | gzip -c \
        | tail -c 8 \
        | od -An -tx4 -N4 \
        | tr -d ' \n')

    if [ -z "${hex_crc}" ]; then
        die "Failed to calculate CRC32"
    fi

    printf '%d' "0x${hex_crc}"
}

# --- Main ---

usage() {
    echo "Usage: $0 <input.bin> <output.bin>"
    echo ""
    echo "This script signs the application header with:"
    echo "  - Firmware size (bytes from offset 0x100 to end)"
    echo "  - CRC32 checksum (of firmware from offset 0x100 to end)"
    echo ""
    echo "Note: CRC excludes header (32 bytes) and padding (224 bytes)."
    echo "The signed binary can then be uploaded via DFU."
    exit 1
}

main() {
    if [ $# -ne 2 ]; then
        usage
    fi

    local input_file="$1"
    local output_file="$2"

    # Check input file exists
    if [ ! -f "${input_file}" ]; then
        die "Input file '${input_file}' not found"
    fi

    # Get file size
    local file_size
    file_size=$(stat -c%s "${input_file}")

    # Check minimum size
    if [ "${file_size}" -lt "${HEADER_SIZE}" ]; then
        die "Binary too small (${file_size} bytes)"
    fi

    # Read and verify magic number (as hex string to avoid signed overflow)
    local magic_hex
    magic_hex=$(read_le32_hex "${input_file}" 0)
    if [ "${magic_hex}" != "${MAGIC}" ]; then
        echo "Error: Invalid magic number 0x${magic_hex} (expected 0xDEADBEEF)" >&2
        exit 1
    fi

    # Read version
    local version
    version=$(read_le32 "${input_file}" 4)

    # Calculate firmware size (from vector table offset to end)
    local firmware_size
    firmware_size=$(( file_size - VECTOR_TABLE_OFFSET ))

    # Calculate CRC32 of firmware data (from vector table offset to end)
    local crc32
    crc32=$(calculate_crc32 "${input_file}" "${VECTOR_TABLE_OFFSET}")

    # Create output: copy input, then overwrite size and CRC32 fields in place
    cp "${input_file}" "${output_file}"
    write_le32 "${output_file}" 8 "${firmware_size}"
    write_le32 "${output_file}" 12 "${crc32}"

    # Calculate total output size (should be same as input)
    local total_size
    total_size=$(stat -c%s "${output_file}")
    local total_kb
    total_kb=$(awk "BEGIN {printf \"%.2f\", ${total_size}/1024}")
    local firmware_kb
    firmware_kb=$(awk "BEGIN {printf \"%.2f\", ${firmware_size}/1024}")

    # Parse version fields
    local ver_major ver_minor ver_patch
    ver_major=$(( (version >> 16) & 0xFFFF ))
    ver_minor=$(( (version >> 8) & 0xFF ))
    ver_patch=$(( version & 0xFF ))

    local padding_size=$(( VECTOR_TABLE_OFFSET - HEADER_SIZE ))

    # Print summary
    echo "============================================================"
    echo "Application Binary Signing Complete"
    echo "============================================================"
    printf 'Input file:       %s\n' "${input_file}"
    printf 'Output file:      %s\n' "${output_file}"
    printf 'Total size:       %d bytes (%s KB)\n' "${total_size}" "${total_kb}"
    printf 'Header size:      %d bytes\n' "${HEADER_SIZE}"
    printf 'Padding size:     %d bytes\n' "${padding_size}"
    printf 'Firmware size:    %d bytes (%s KB)\n' "${firmware_size}" "${firmware_kb}"
    printf 'Magic:            0x%s\n' "${magic_hex}"
    printf 'Version:          0x%08X (%d.%d.%d)\n' "${version}" "${ver_major}" "${ver_minor}" "${ver_patch}"
    printf 'CRC32:            0x%08X\n' "${crc32}"
    echo "============================================================"

    return 0
}

main "$@"
