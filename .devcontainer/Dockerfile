FROM ubuntu:24.04

# Install any additional tools you might want
RUN apt-get update && apt-get upgrade -y

# usbutils - To check for the USB connection (Run: lsusb). https://packages.ubuntu.com/noble/usbutils
RUN apt-get install usbutils -y 

# git - For getting repositories. https://packages.ubuntu.com/noble/git
RUN apt-get install git -y

# make - Compile C files. https://packages.ubuntu.com/noble/make
RUN apt-get install make -y 
# build-essential - Includes dpkg-dev, g++, gcc, libc6-dev, make. https://packages.ubuntu.com/noble/build-essential
#RUN apt-get install build-essential -y

# gcc-arm-none-eabi - Arm GNU Toolchain. Includes libnewlib aswell. https://packages.ubuntu.com/noble/gcc-arm-none-eabi
# TODO: See if I should use the gcc-arm-none-eabi that ChibiStudio includes under tools (2 versions: GCC 10.3.1 2021q4, GCC 11.3.1 2022q3)
# The newest one as of 19.12.2025 is now GCC 13.2.rel1
RUN apt-get install gcc-arm-none-eabi -y

# openocd - Open On-chip Debugger. https://packages.ubuntu.com/noble/openocd
#RUN apt-get install openocd -y
# Temporary solution to use the newest (dev) version of OpenOCD from source.
#RUN apt install build-essential autoconf automake libtool pkg-config texinfo libusb-1.0-0-dev libhidapi-dev libhidapi-libusb0 libftdi-dev libftdi1 libjim-dev
RUN apt install autoconf automake libtool pkg-config libusb-1.0-0-dev libhidapi-dev libftdi-dev libftdi1 libjim-dev -y
RUN git clone --recursive https://github.com/openocd-org/openocd.git /openocd
#RUN cd /openocd
# Use WORKDIR instead of RUN cd /openocd, because how Docker isn't properly assigned to the correct directory with the "RUN cd"-command.
WORKDIR /openocd
RUN ./bootstrap
RUN ./configure --enable-ftdi --enable-stlink --enable-cmsis-dap --enable-cmsis-dap-v2 --prefix=/usr
# Without --prefix=/usr, openocd will be set to /usr/local/bin and config files to /usr/local/share/openocd by default, hence changing it to /usr.
#RUN make
RUN make -j$(nproc)
RUN make install
#RUN cd /
WORKDIR /
#RUN rm -rf /openocd

# gdb - GNU project Debugger. https://packages.ubuntu.com/noble/gdb
#RUN apt-get install gdb -y
# gdb - GNU project Debugger. https://packages.ubuntu.com/noble/gdb
RUN apt-get install gdb-multiarch -y

# minicom - Serial communication program. https://packages.ubuntu.com/noble/minicom
RUN apt-get install minicom -y

# Clean up
RUN rm -rf /var/lib/apt/lists/*

# Make workspace directory
RUN mkdir /workspace



# NB! Test first to see if it works. Might be conflicting with how the folder is created with devcontainer.json.
# Making changes in that folder useless from the Dockerfile.

# Get ChibiOS Repository from Github
#RUN git clone https://github.com/ChibiOS/ChibiOS /workspace/ChibiOS


WORKDIR /workspace
