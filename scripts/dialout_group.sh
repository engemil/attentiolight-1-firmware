#!/bin/bash

# dialout_group.sh - Script to add current user to the dialout group.
# This script is necessary for allowing non-root users to access serial devices (e.g. Arduino) on Linux systems.
#
# HOW-TO use:
# 1. Configure permissions: sudo chmod +x ./dialout_group.sh
# 2. Execute: sudo ./dialout_group.sh
#
# Requirements: Must be run with sudo privileges.

# Check if script is run with sudo
if [ "$EUID" -ne 0 ]; then
    echo "Error: This script must be run as root (use sudo)."
    exit 1
fi

# Step 1: Add current user to the dialout group
usermod -a -G dialout $USER

# Done
echo "Please log out and log back in to apply the group changes."

# Exit the script successfully
exit 0



