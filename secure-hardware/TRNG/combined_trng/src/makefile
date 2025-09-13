# Final output folder
OUTPUT = build
OBJDIR = $(OUTPUT)
TARGET = $(OUTPUT)/combined_trng

# Auto-create output folders
$(shell mkdir -p $(OUTPUT)/src)

# Source files
SRC = $(wildcard *.c)

# Simpleserial
SS_VER = SS_VER_2_1

# Extra options
EXTRA_OPTS = NO_EXTRA_OPTS
CFLAGS += -D$(EXTRA_OPTS)

ifeq ($(CRYPTO_TARGET),)
  $(info No CRYPTO_TARGET passed - defaulting to TINYAES128C)
  CRYPTO_TARGET = TINYAES128C
endif

$(info Building for platform $(PLATFORM) with CRYPTO_TARGET=$(CRYPTO_TARGET))

# Add simpleserial project
include ../../../../firmware/mcu/simpleserial/Makefile.simpleserial

# Path to CW firmware core
FIRMWAREPATH = ../../../../firmware/mcu
include $(FIRMWAREPATH)/Makefile.inc