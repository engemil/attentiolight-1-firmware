# List of all the board related files.
BOARDSRC = $(BOARDSDIR)/ST_NUCLEO64_C071RB/board.c

# Required include directories
BOARDINC = $(BOARDSDIR)/ST_NUCLEO64_C071RB/


# Shared variables
ALLCSRC += $(BOARDSRC)
ALLINC  += $(BOARDINC)