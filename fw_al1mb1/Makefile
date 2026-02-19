##############################################################################
# Build global options
# NOTE: Can be overridden externally.
#

# Compiler options here.
ifeq ($(USE_OPT),)
  USE_OPT = -O2 -ggdb -fomit-frame-pointer -falign-functions=16
endif

# C specific options here (added to USE_OPT).
ifeq ($(USE_COPT),)
  USE_COPT = 
endif

# C++ specific options here (added to USE_OPT).
ifeq ($(USE_CPPOPT),)
  USE_CPPOPT = -fno-rtti
endif

# Enable this if you want the linker to remove unused code and data.
ifeq ($(USE_LINK_GC),)
  USE_LINK_GC = yes
endif

# Linker extra options here.
ifeq ($(USE_LDOPT),)
  USE_LDOPT = 
endif

# Enable this if you want link time optimizations (LTO).
ifeq ($(USE_LTO),)
  USE_LTO = yes
endif

# Enable this if you want to see the full log while compiling.
ifeq ($(USE_VERBOSE_COMPILE),)
  USE_VERBOSE_COMPILE = no
endif

# If enabled, this option makes the build process faster by not compiling
# modules not used in the current configuration.
ifeq ($(USE_SMART_BUILD),)
  USE_SMART_BUILD = yes
endif

#
# Build global options
##############################################################################

##############################################################################
# Architecture or project specific options
#

# Stack size to be allocated to the Cortex-M process stack. This stack is
# the stack used by the main() thread.
ifeq ($(USE_PROCESS_STACKSIZE),)
  USE_PROCESS_STACKSIZE = 0x400
endif

# Stack size to the allocated to the Cortex-M main/exceptions stack. This
# stack is used for processing interrupts and exceptions.
ifeq ($(USE_EXCEPTIONS_STACKSIZE),)
  USE_EXCEPTIONS_STACKSIZE = 0x400
endif

# Enables the use of FPU (no, softfp, hard).
ifeq ($(USE_FPU),)
  USE_FPU = no
endif

# FPU-related options.
ifeq ($(USE_FPU_OPT),)
  USE_FPU_OPT = -mfloat-abi=$(USE_FPU) -mfpu=fpv4-sp-d16
endif

#
# Architecture or project specific options
##############################################################################

##############################################################################
# Project, target, sources and paths
#

# Define project name here
PROJECT = fw_al1mb1

# Target settings.
MCU  = cortex-m0plus

# Imported source files and paths.
#CHIBIOS  := ./chibios
CHIBIOS  := ../ext/ChibiOS
CONFDIR  := ./cfg
BUILDDIR := ./build
DEPDIR   := ./.dep
BOARDSDIR := ./boards

# Licensing files.
include $(CHIBIOS)/os/license/license.mk

# Startup files.
include $(CHIBIOS)/os/common/startup/ARMCMx/compilers/GCC/mk/startup_stm32c0xx.mk

# HAL files (optional)
include $(CHIBIOS)/os/hal/hal.mk
include $(CHIBIOS)/os/hal/ports/STM32/STM32C0xx/platform.mk

# Board files. Only uncomment one of the following lines, depending on the board you are using.
#include $(CHIBIOS)/os/hal/boards/ST_NUCLEO64_C071RB/board.mk
include $(BOARDSDIR)/AL1_MB1/board.mk
#include $(BOARDSDIR)/ST_NUCLEO64_C071RB/board.mk

# HAL-OSAL files (optional).
include $(CHIBIOS)/os/hal/osal/rt-nil/osal.mk
# RTOS files (optional).
include $(CHIBIOS)/os/rt/rt.mk
include $(CHIBIOS)/os/common/ports/ARMv6-M/compilers/GCC/mk/port.mk

# Auto-build files in ./source recursively.
include $(CHIBIOS)/tools/mk/autobuild.mk

# Streams files (optional).
include $(CHIBIOS)/os/hal/lib/streams/streams.mk

# ChibiOS test files (optional).
#include $(CHIBIOS)/os/test/test.mk
#include $(CHIBIOS)/test/rt/rt_test.mk
#include $(CHIBIOS)/test/oslib/oslib_test.mk

# Project Libraries
LIBS     := ./libs
include $(LIBS)/libs.mk

# Define linker script file here
# ChibiOS provides a default linker script for STM32C0xx devices, but we use 
#LDSCRIPT= $(STARTUPLD)/STM32C071xB.ld
LDSCRIPT= STM32C071xB_bootloader.ld

# C sources that can be compiled in ARM or THUMB mode depending on the global
# setting.
CSRC = $(ALLCSRC) \
       $(TESTSRC) \
       app/main.c \
       app/button_driver/button_driver.c \
       app/ws2812b_led_driver/ws2812b_led_driver.c \
       app/app_state_machine/app_state_machine.c \
       app/app_state_machine/animation/animation_thread.c \
       app/app_state_machine/animation/animation_helpers.c \
       app/app_state_machine/modes/effects/effect_breathing.c \
       app/app_state_machine/modes/effects/effect_candle.c \
       app/app_state_machine/modes/effects/effect_fire.c \
       app/app_state_machine/modes/effects/effect_lava_lamp.c \
       app/app_state_machine/modes/effects/effect_day_night.c \
       app/app_state_machine/modes/effects/effect_ocean.c \
       app/app_state_machine/modes/effects/effect_northern_lights.c \
       app/app_state_machine/modes/effects/effect_thunder_storm.c \
       app/app_state_machine/modes/effects/effect_police.c \
       app/app_state_machine/modes/effects/effect_health_pulse.c \
       app/app_state_machine/modes/effects/effect_memory.c \
       app/app_state_machine/modes/effects/effect_rainbow.c \
       app/app_state_machine/modes/effects/effect_color_cycle.c \
       app/app_state_machine/system_states/state_boot.c \
       app/app_state_machine/system_states/state_powerup.c \
       app/app_state_machine/system_states/state_active.c \
       app/app_state_machine/system_states/state_powerdown.c \
       app/app_state_machine/system_states/state_off.c \
       app/app_state_machine/modes/modes.c \
       app/app_state_machine/modes/mode_solid_color.c \
       app/app_state_machine/modes/mode_brightness.c \
       app/app_state_machine/modes/mode_blinking.c \
       app/app_state_machine/modes/mode_pulsation.c \
       app/app_state_machine/modes/mode_effects.c \
       app/app_state_machine/modes/mode_traffic_light.c \
       app/app_state_machine/modes/mode_night_light.c \
       app/app_state_machine/modes/mode_external_control.c \
       app_header/app_header.c

# C++ sources that can be compiled in ARM or THUMB mode depending on the global
# setting.
CPPSRC = $(ALLCPPSRC)

# List ASM source files here.
ASMSRC = $(ALLASMSRC)

# List ASM with preprocessor source files here.
ASMXSRC = $(ALLXASMSRC)

# Inclusion directories.
INCDIR = $(CONFDIR) $(ALLINC) $(TESTINC)

# Define C warning options here.
CWARN = -Wall -Wextra -Wundef -Wstrict-prototypes -Wcast-align=strict

# Define C++ warning options here.
CPPWARN = -Wall -Wextra -Wundef

#
# Project, target, sources and paths
##############################################################################

##############################################################################
# Start of user section
#

# List all user C define here, like -D_DEBUG=1
UDEFS = -DCRT0_VTOR_INIT=1

# Define ASM defines here
UADEFS = -DCRT0_VTOR_INIT=1

# List all user directories here
UINCDIR = ./app_header \
          ./app \
          ./app/button_driver \
          ./app/ws2812b_led_driver \
          ./app/app_state_machine \
          ./app/app_state_machine/animation \
          ./app/app_state_machine/system_states \
          ./app/app_state_machine/modes \
          ./app/app_state_machine/modes/effects

# List the user directory to look for the libraries here
ULIBDIR =

# List all user libraries here
ULIBS =

#
# End of user section
##############################################################################

##############################################################################
# Common rules
#

RULESPATH = $(CHIBIOS)/os/common/startup/ARMCMx/compilers/GCC/mk
include $(RULESPATH)/arm-none-eabi.mk
include $(RULESPATH)/rules.mk

#
# Common rules
##############################################################################

##############################################################################
# Custom rules
#

# Path to signing script
SIGN_SCRIPT = ../scripts/sign_app_header.sh

# Sign binary with size and CRC32 (runs automatically after build)
POST_MAKE_ALL_RULE_HOOK: sign_firmware

sign_firmware: $(BUILDDIR)/$(PROJECT).bin
	@echo ""
	@echo "=========================================="
	@echo "Signing firmware with size and CRC32..."
	@echo "=========================================="
	@bash $(SIGN_SCRIPT) $(BUILDDIR)/$(PROJECT).bin $(BUILDDIR)/$(PROJECT)_signed.bin
	@echo ""
	@echo "✓ Build and signing complete!"
	@echo "  Unsigned: $(BUILDDIR)/$(PROJECT).bin (do not upload)"
	@echo "  Signed:   $(BUILDDIR)/$(PROJECT)_signed.bin (ready to upload)"
	@echo ""

.PHONY: sign_firmware

##############################################################################
# Debug build targets
#
# Usage:
#   make                  - Normal release build (no debug output)
#   make debug            - Debug build with INFO level (default LEVEL=3)
#   make debug LEVEL=1    - Errors only
#   make debug LEVEL=2    - Errors + warnings
#   make debug LEVEL=3    - Errors + warnings + info (state/mode changes)
#   make debug LEVEL=4    - Everything (verbose)
#   make release          - Explicit release build (no debug)
#
# Debug levels are hierarchical:
#   0 = NONE  - No output (release build)
#   1 = ERROR - Critical errors only
#   2 = WARN  - Errors + warnings
#   3 = INFO  - + state/mode changes
#   4 = DEBUG - + verbose details
#

.PHONY: debug release clean-build

# Debug build - forces clean rebuild with debug flags
# Default LEVEL=3 (INFO) if not specified
debug: clean-build
	$(MAKE) all UDEFS="$(UDEFS) -DAPP_DEBUG_LEVEL=$(if $(LEVEL),$(LEVEL),3)"

# Explicit release build (no debug output)
release: clean-build
	$(MAKE) all UDEFS="$(UDEFS) -DAPP_DEBUG_LEVEL=0"

# Clean build directory for fresh rebuild
clean-build:
	@echo "Cleaning build directory for fresh rebuild..."
	@rm -rf $(BUILDDIR)
	@rm -rf $(DEPDIR)

#
# Custom rules
##############################################################################