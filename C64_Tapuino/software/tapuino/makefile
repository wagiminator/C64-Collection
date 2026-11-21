# ===================================================================================
# Project Makefile
# ===================================================================================
# Project:  Tapuino - Commodore Datasette Emulator
# Author:   Stefan Wagner
# Year:     2026
# URL:      https://github.com/wagiminator
# ===================================================================================
# Install toolchain:
#   sudo apt install build-essential gcc-avr avr-libc avrdude
#
# Connect an ISP programmer (e.g. USBasp) to the ICSP header of the Tapuino and
# type "make flash" in the command line. "make fuses" sets the fuses once (16MHz
# crystal, brown-out detection at 2.7V).
# ===================================================================================

# Files and Folders
TARGET   = tapuino
SOURCE   = src
BIN      = bin

# Microcontroller Settings
DEVICE   = atmega328p
F_CPU    = 16000000
LFUSE    = 0xFF
HFUSE    = 0xDB
EFUSE    = 0xFD

# Programmer Settings (see "avrdude -c ?" for a list of programmers)
PROGRMR ?= usbasp

# Toolchain
PREFIX   = avr
CC       = $(PREFIX)-gcc
OBJCOPY  = $(PREFIX)-objcopy
OBJDUMP  = $(PREFIX)-objdump
OBJSIZE  = $(PREFIX)-size
ISPTOOL  = avrdude -c $(PROGRMR) -p $(DEVICE)
CLEAN    = rm -f *.lst *.obj *.cof *.list *.map *.eep.hex *.o *.d

# Compiler Flags
CFLAGS   = -Os -flto -Wall -mmcu=$(DEVICE) -DF_CPU=$(F_CPU) -ffunction-sections -fdata-sections
CFLAGS  += -mcall-prologues -fno-tree-switch-conversion -I$(SOURCE) -I.
LDFLAGS  = -Wl,--gc-sections
CFILES   = $(wildcard ./*.c) $(wildcard $(SOURCE)/*.c)

.PHONY: help all elf bin hex asm flash fuses clean size removetemp removeelf

# Symbolic Targets
help:
	@echo "Use the following commands:"
	@echo "make all       compile and build $(TARGET).elf/.bin/.hex/.asm"
	@echo "make hex       compile and build $(TARGET).hex"
	@echo "make asm       compile and disassemble to $(TARGET).asm"
	@echo "make bin       compile and build $(TARGET).bin"
	@echo "make flash     compile and upload to MCU"
	@echo "make fuses     write fuses to MCU"
	@echo "make clean     remove all build files"

$(BIN)/$(TARGET).elf: $(CFILES) config.h $(wildcard $(SOURCE)/*.h)
	@echo "Building $(BIN)/$(TARGET).elf ..."
	@mkdir -p $(BIN)
	@$(CC) -o $@ $(CFILES) $(CFLAGS) $(LDFLAGS)

$(BIN)/$(TARGET).lst: $(BIN)/$(TARGET).elf
	@echo "Building $(BIN)/$(TARGET).lst ..."
	@$(OBJDUMP) -S $^ > $(BIN)/$(TARGET).lst

$(BIN)/$(TARGET).map: $(BIN)/$(TARGET).elf
	@echo "Building $(BIN)/$(TARGET).map ..."
	@$(OBJDUMP) -t $^ > $(BIN)/$(TARGET).map

$(BIN)/$(TARGET).bin: $(BIN)/$(TARGET).elf
	@echo "Building $(BIN)/$(TARGET).bin ..."
	@$(OBJCOPY) -O binary -R .eeprom $< $(BIN)/$(TARGET).bin

$(BIN)/$(TARGET).hex: $(BIN)/$(TARGET).elf
	@echo "Building $(BIN)/$(TARGET).hex ..."
	@$(OBJCOPY) -O ihex -R .eeprom $< $(BIN)/$(TARGET).hex

$(BIN)/$(TARGET).asm: $(BIN)/$(TARGET).elf
	@echo "Disassembling to $(BIN)/$(TARGET).asm ..."
	@$(OBJDUMP) -d $(BIN)/$(TARGET).elf > $(BIN)/$(TARGET).asm

all:	$(BIN)/$(TARGET).lst $(BIN)/$(TARGET).map $(BIN)/$(TARGET).bin $(BIN)/$(TARGET).hex $(BIN)/$(TARGET).asm size

elf:	$(BIN)/$(TARGET).elf removetemp size

bin:	$(BIN)/$(TARGET).bin removetemp size removeelf

hex:	$(BIN)/$(TARGET).hex removetemp size removeelf

asm:	$(BIN)/$(TARGET).asm removetemp size removeelf

flash:	$(BIN)/$(TARGET).hex size removeelf
	@echo "Uploading to $(DEVICE) ..."
	@$(ISPTOOL) -U flash:w:$(BIN)/$(TARGET).hex:i

fuses:
	@echo "Writing fuses to $(DEVICE) ..."
	@$(ISPTOOL) -U lfuse:w:$(LFUSE):m -U hfuse:w:$(HFUSE):m -U efuse:w:$(EFUSE):m

clean:
	@echo "Cleaning all up ..."
	@$(CLEAN)
	@rm -f $(BIN)/$(TARGET).elf $(BIN)/$(TARGET).lst $(BIN)/$(TARGET).map $(BIN)/$(TARGET).bin $(BIN)/$(TARGET).hex $(BIN)/$(TARGET).asm

size:
	@echo "------------------"
	@echo "FLASH: $(shell $(OBJSIZE) -d $(BIN)/$(TARGET).elf | awk '/[0-9]/ {print $$1 + $$2}') bytes"
	@echo "SRAM:  $(shell $(OBJSIZE) -d $(BIN)/$(TARGET).elf | awk '/[0-9]/ {print $$2 + $$3}') bytes"
	@echo "------------------"

removetemp:
	@echo "Removing temporary files ..."
	@$(CLEAN)

removeelf:
	@echo "Removing $(BIN)/$(TARGET).elf ..."
	@rm -f $(BIN)/$(TARGET).elf
