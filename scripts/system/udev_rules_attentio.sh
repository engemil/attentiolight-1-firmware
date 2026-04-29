#!/bin/bash

# udev_rules_attentio.sh - Script to set up udev rules for STMicroelectronics ST-LINK devices
# and Attentio USB devices in Linux (e.g., Ubuntu).
# This script creates udev rules to allow non-root users to access the ST-LINK and
# Attentio USB devices. It also adds the current user to the 'plugdev' group,
# which is necessary for USB device access.
#
# Supports ST-LINK/V2-1, ST-LINK/V3E, ST-LINK/V2EC, ST-LINK/V3EC, STM32 DFU mode,
# and Attentio devices (VID:PID 1209:EEA1 from pid.codes).
#
# HOW-TO use:
# 1. Configure permissions: sudo chmod +x ./udev_rules_attentio.sh
# 2. Execute: sudo ./udev_rules_attentio.sh
#
# Requirements: Must be run with sudo privileges.

# Check if script is run with sudo
if [ "$EUID" -ne 0 ]; then
    echo "Error: This script must be run as root (use sudo)."
    exit 1
fi

# Step 1: Create udev rules file
# Source: https://www.st.com/resource/en/technical_note/tn1235-overview-of-stlink-derivatives-stmicroelectronics.pdf
# If you share your linux system with other users, or just don't like the idea of giving write permissions to everyone, you can change the MODE to 0660
# and change the GROUP to a specific group that you want to allow access to the ST-LINK devices. 
# For example, you can create a group called 'stlink' and change the GROUP to 'stlink'.
# Note: The SYMLINK+="stlinkv2_1_%n" creates a symlink in /dev with the name stlinkv2_1_<bus>_<device> for easy access.

UDEV_RULES_FILE="/etc/udev/rules.d/49-stlink.rules"
echo "Creating udev rules in $UDEV_RULES_FILE..."

echo ""

echo "Content of $UDEV_RULES_FILE:"

echo ""

cat << EOF | tee $UDEV_RULES_FILE
# STMicroelectronics ST-LINK/V2-1 (Nucleo/Discovery boards)
SUBSYSTEMS=="usb", ATTRS{idVendor}=="0483", ATTRS{idProduct}=="374b", MODE="0666", GROUP="plugdev", SYMLINK+="stlinkv2_1_%n"

# STMicroelectronics ST-LINK/V2-1 (Nucleo/Discovery boards)
SUBSYSTEMS=="usb", ATTRS{idVendor}=="0483", ATTRS{idProduct}=="3752", MODE="0666", GROUP="plugdev", SYMLINK+="stlinkv2_1_%n"

# STMicroelectronics ST-LINK/V3 w/o bridge functions
SUBSYSTEMS=="usb", ATTRS{idVendor}=="0483", ATTRS{idProduct}=="374e", MODE="0666", GROUP="plugdev", SYMLINK+="stlinkv3e_%n"

# STMicroelectronics ST-LINK/V3 w/o bridge functions
SUBSYSTEMS=="usb", ATTRS{idVendor}=="0483", ATTRS{idProduct}=="3754", MODE="0666", GROUP="plugdev", SYMLINK+="stlinkv3e_%n"

# STMicroelectronics ST-LINK/V3 w/ bridge functions
SUBSYSTEMS=="usb", ATTRS{idVendor}=="0483", ATTRS{idProduct}=="374f", MODE="0666", GROUP="plugdev", SYMLINK+="stlinkv3e_%n"

# STMicroelectronics ST-LINK/V3 w/ bridge functions
SUBSYSTEMS=="usb", ATTRS{idVendor}=="0483", ATTRS{idProduct}=="3753", MODE="0666", GROUP="plugdev", SYMLINK+="stlinkv3e_%n"

# STMicroelectronics ST-LINK/V3PWR
SUBSYSTEMS=="usb", ATTRS{idVendor}=="0483", ATTRS{idProduct}=="3757", MODE="0666", GROUP="plugdev", SYMLINK+="stlinkv3ec_%n"

# STMicroelectronics STM32 Chip in DFU mode (also used as Attentio bootloader fallback
# when no valid application header is present on a blank/erased board)
SUBSYSTEMS=="usb", ATTRS{idVendor}=="0483", ATTRS{idProduct}=="df11", MODE="0666", GROUP="plugdev", SYMLINK+="stm32c0_%n"

# Attentio USB Device (VID:PID from pid.codes, https://pid.codes/1209/EEA1/)
SUBSYSTEM=="usb", ATTRS{idVendor}=="1209", ATTRS{idProduct}=="eea1", MODE="0666", GROUP="plugdev", ENV{ID_MM_DEVICE_IGNORE}="1"
SUBSYSTEM=="tty", ATTRS{idVendor}=="1209", ATTRS{idProduct}=="eea1", MODE="0666", GROUP="plugdev", SYMLINK+="attentio-%s{serial}"
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
echo "You can verify device detection with 'lsusb'. Expected IDs:"
echo "  ST-LINK/V2-1:       0483:374b"
echo "  ST-LINK/V2-1:       0483:3752"
echo "  ST-LINK/V3 w/o BF:  0483:374e"
echo "  ST-LINK/V3 w/o BF:  0483:3754"
echo "  ST-LINK/V3 w/ BF:   0483:374f"
echo "  ST-LINK/V3 w/ BF:   0483:3753"
echo "  ST-LINK/V3PWR:      0483:3757"
echo "  STM32 DFU:          0483:df11"
echo "  Attentio:           1209:eea1"

echo ""

# Optional: Check connected ST-LINK and Attentio devices
echo "Checking for connected ST-LINK and Attentio devices with lsusb..."
lsusb | grep -E "0483:.*(374b|3752|374e|3754|3753|3757|df11)|1209:eea1" || echo "No ST-LINK or Attentio devices detected. Ensure the device is connected."

echo ""

# Done
echo "Please log out and log back in to apply the group changes."

# Exit the script successfully
exit 0
