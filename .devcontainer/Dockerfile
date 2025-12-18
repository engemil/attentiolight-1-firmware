FROM ubuntu:24.04

# Install any additional tools you might want
RUN apt update && apt upgrade -y

# usbutils - To check for the USB connection (Run: lsusb). https://packages.ubuntu.com/noble/usbutils
RUN apt install usbutils -y 

# git - For getting repositories. https://packages.ubuntu.com/noble/git
RUN apt install git -y

# make - Compile C files. https://packages.ubuntu.com/noble/make
RUN apt install make -y 
# build-essential - Includes dpkg-dev, g++, gcc, libc6-dev, make. https://packages.ubuntu.com/noble/build-essential
#RUN apt install build-essential -y

# gcc-arm-none-eabi - Arm GNU Toolchain. Includes libnewlib aswell. https://packages.ubuntu.com/noble/gcc-arm-none-eabi
# TODO: See if I should use the gcc-arm-none-eabi that ChibiStudio includes under tools (2 versions: GCC 10.3.1 2021q4, GCC 11.3.1 2022q3)
# The newest one as of 19.12.2025 is now GCC 13.2.rel1
RUN apt install gcc-arm-none-eabi -y

# openocd - Open On-chip Debugger. https://packages.ubuntu.com/noble/openocd
#RUN apt install openocd -y
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
#RUN apt install gdb -y
# gdb - GNU project Debugger. https://packages.ubuntu.com/noble/gdb
RUN apt install gdb-multiarch -y


#### FOR ESP ####
# https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/linux-macos-setup.html

RUN apt install git wget flex bison gperf python3 python3-pip python3-venv cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0 -y

#RUN mkdir -p /esp-idf
#RUN cd /esp-idf
RUN cd /
RUN git clone --single-branch --recursive https://github.com/espressif/esp-idf.git esp-idf

RUN cd /esp-idf
#RUN ./install.sh all
RUN ./install.sh esp32,esp32c3

# Set ESP-IDF environment variables? (should I activate this when?)
#RUN . ./export.sh

# Run this in the terminal you are going to use: . /esp-idf/export.sh
# Alternatively, add this:
# alias get_idf='. $HOME/esp/esp-idf/export.sh'
# or add this:
# alias get_idf='. /esp-idf/export.sh'
# in the ~/.profile, ~/.bashrc, ~/.zshrc or ~/.zprofile, etc.. file to source it automatically when opening a terminal.
# Then run: source ~/.bashrc (or the respective file you added the line to)


#################

# minicom - Serial communication program. https://packages.ubuntu.com/noble/minicom
RUN apt install minicom -y


# Clean up
RUN rm -rf /var/lib/apt/lists/*





# Make workspace directory
RUN mkdir /workspace



# NB! Test first to see if it works. Might be conflicting with how the folder is created with devcontainer.json.
# Making changes in that folder useless from the Dockerfile.

# Get ChibiOS Repository from Github
#RUN git clone https://github.com/ChibiOS/ChibiOS /workspace/ChibiOS


WORKDIR /workspace
