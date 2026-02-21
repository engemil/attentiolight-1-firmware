# Base path for libraries
LIBS_DIR := $(LIBS)

# List of library directories (add more as needed)
LIB_DIRS := usbcfg portab ee_esp32_wifi_ble_if_driver hal_efl_stm32c0xx

# Collect all source files from library directories
LIBSSRC := $(foreach dir,$(LIB_DIRS),$(wildcard $(LIBS_DIR)/$(dir)/*.c))

# Collect all include directories
LIBSINC := $(foreach dir,$(LIB_DIRS),$(LIBS_DIR)/$(dir))

# Add to shared variables for ChibiOS build system
ALLCSRC += $(LIBSSRC)
ALLINC  += $(LIBSINC)