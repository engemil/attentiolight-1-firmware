# Base path for the EngEmil PMW3901MB DRIVER
EE_PMW3901MB_DRIV := $(LIBS)/engemil_pmw3901mb_driver

# List of all Source files
LIBSSRC := $(wildcard $(EE_PMW3901MB_DRIV)/src/*.c)

# Required include directories
LIBSINC := $(EE_PMW3901MB_DRIV)/include/

# Add to shared variables for ChibiOS build system
ALLCSRC += $(LIBSSRC)
ALLINC  += $(LIBSINC)