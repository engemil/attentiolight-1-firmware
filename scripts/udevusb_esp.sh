#!/bin/bash

# udevusb_esp.sh - Script to set up udev rules for Espressif ESP debugger in Linux (e.g., Ubuntu)
# This script creates a udev rule to allow non-root users to access the ESP / ESP-Prog USB device(s).
# It also adds the current user to the 'plugdev' group.
#
# Supports: ESP-Prog-2
#
# HOW-TO use:
# 1. Configure permissions: sudo chmod +x ./udevusb_esp.sh
# 2. Execute: sudo ./udevusb_esp.sh
#
# Requirements: Must be run with sudo privileges.

# Check if script is run with sudo
if [ "$EUID" -ne 0 ]; then
    echo "Error: This script must be run as root (use sudo)."
    exit 1
fi

# Step 1: Create udev rules file
UDEV_RULES_FILE="/etc/udev/rules.d/50-esp.rules"
echo "Creating udev rules in $UDEV_RULES_FILE..."

echo ""

echo "Content of $UDEV_RULES_FILE:"

echo ""

cat << EOF | tee $UDEV_RULES_FILE
# Espressif ESP-Prog-2 (USB-JTAG/Serial bridge)
SUBSYSTEMS=="usb", ATTRS{idVendor}=="303a", ATTRS{idProduct}=="1002", MODE="0666", GROUP="plugdev", SYMLINK+="esp_%n"
EOF

echo ""

# Step 2: Set permissions for udev rules file
chmod 644 $UDEV_RULES_FILE

# Step 3: Reload udev rules
echo "Reloading udev rules..."
udevadm control --reload-rules
udevadm trigger

# Step 4: Add user to plugdev and dialout group
if [ -n "$SUDO_USER" ]; then
    echo "Adding user $SUDO_USER to plugdev and dialout group..."
    usermod -aG plugdev "$SUDO_USER"
    usermod -a -G dialout "$SUDO_USER"
else
    echo "Warning: Could not determine user (SUDO_USER not set). Skipping group addition."
fi

echo ""

echo "Script COMPLETED successfully!"

echo ""

# Step 5: Inform the user
echo "You can verify device detection with 'lsusb'. Expected ID:"
echo " ESP-Prog-2: 303a:1002"

echo ""

# Optional: Check connected device
echo "Checking for connected ESP-Prog-2 with lsusb..."
lsusb | grep "303a:" | grep -E "1001|1002" || echo "No ESP device(s) detected. Ensure the device is connected."

echo ""

# Done
echo "Please log out and log back in to apply the group changes."

# Exit the script successfully
exit 0
