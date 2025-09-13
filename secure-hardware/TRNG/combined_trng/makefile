# Put the final files in this folder
OUTPUT = build

# Auto-create build directory if it doesn’t exist
$(shell mkdir -p $(OUTPUT))

# Put object files in build/ as well
OBJDIR = $(OUTPUT)

# Final build file
TARGET = combined_trng

# Auto-discover all C firmware sources in src/
SRC = $(wildcard src/*.c)

# Use simpleserial V2
SS_VER = SS_VER_2_1

# Extra options
EXTRA_OPTS = NO_EXTRA_OPTS
CFLAGS += -D$(EXTRA_OPTS)

# Default crypto target (not used, but required by CW build system)
ifeq ($(CRYPTO_TARGET),)
  ${info No CRYPTO_TARGET passed - defaulting to TINYAES128C}
  CRYPTO_TARGET = TINYAES128C
endif

${info Building for platform ${PLATFORM} with CRYPTO_TARGET=$(CRYPTO_TARGET)}

# Add simpleserial project to build
include ../../../firmware/mcu/simpleserial/Makefile.simpleserial

# Path to ChipWhisperer firmware core
FIRMWAREPATH = ../../../firmware/mcu
include $(FIRMWAREPATH)/Makefile.inc
