# ===================================================================================
# Project Makefile
# ===================================================================================
# Project:  SD2IEC - Commodore Floppy Disk Drive Emulator
# Author:   Stefan Wagner
# Year:     2026
# URL:      https://github.com/wagiminator
# ===================================================================================
# Install toolchain:
#   sudo apt install build-essential gcc-avr avr-libc avrdude python3
#
# Connect an ISP programmer (e.g. USBasp) to the ICSP header of the SD2IEC (remove
# the SD card, do not connect the SD2IEC to the computer while flashing) and type
# "make flash" in the command line. "make fuses" sets the fuses once.
#
# "make update" builds sd2iec.bin for the newboot bootloader: copy it to the root of
# the SD card, it is flashed automatically at the next power-up.
#
# All firmware files (bin and hex) contain the boot information (device ID and CRC)
# at the end of the application area: the bootloader only starts a firmware with a
# valid CRC.
# ===================================================================================

# Files and Folders
TARGET   = sd2iec
SOURCE   = src
BIN      = bin
BOOTHEX  = $(BIN)/newboot-0.4.1-larsp-m644p.hex

# Microcontroller Settings
DEVICE   = atmega644p
F_CPU    = 8000000
LFUSE    = 0xEF
HFUSE    = 0xD2
EFUSE    = 0xFD
APPSIZE  = 0xF000

# Programmer Settings (see "avrdude -c ?" for a list of programmers)
PROGRMR ?= usbasp

# Toolchain
PREFIX   = avr
CC       = $(PREFIX)-gcc
OBJCOPY  = $(PREFIX)-objcopy
OBJDUMP  = $(PREFIX)-objdump
OBJSIZE  = $(PREFIX)-size
ISPTOOL  = avrdude -c $(PROGRMR) -p $(DEVICE)
PYTHON   = python3
CLEAN    = rm -f *.lst *.obj *.cof *.list *.map *.eep.hex *.o *.d

# Compiler Flags
CFLAGS   = -Os -flto -Wall -mmcu=$(DEVICE) -DF_CPU=$(F_CPU) -ffunction-sections -fdata-sections
CFLAGS  += -mcall-prologues -mrelax -fno-tree-switch-conversion -I$(SOURCE) -I.
LDFLAGS  = -Wl,--gc-sections -Wl,--relax
CFILES   = $(wildcard ./*.c) $(wildcard $(SOURCE)/*.c) $(wildcard $(SOURCE)/*.S)

.PHONY: help all elf bin hex asm flash fuses update clean size removetemp removeelf

# Symbolic Targets
help:
	@echo "Use the following commands:"
	@echo "make all       compile and build $(TARGET).elf/.bin/.hex/.asm"
	@echo "make hex       compile and build $(TARGET).hex"
	@echo "make asm       compile and disassemble to $(TARGET).asm"
	@echo "make update    build $(TARGET).bin for the bootloader (firmware update via SD)"
	@echo "make flash     compile and upload firmware and bootloader to MCU"
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
	@$(PYTHON) tools/bootinfo.py $(BIN)/$(TARGET).bin $(APPSIZE) config.h

$(BIN)/$(TARGET).hex: $(BIN)/$(TARGET).bin
	@echo "Building $(BIN)/$(TARGET).hex ..."
	@$(PYTHON) tools/mergehex.py $(BIN)/$(TARGET).hex $(BIN)/$(TARGET).bin

$(BIN)/$(TARGET)_boot.hex: $(BIN)/$(TARGET).bin $(BOOTHEX)
	@echo "Building $(BIN)/$(TARGET)_boot.hex (firmware + bootloader) ..."
	@$(PYTHON) tools/mergehex.py $(BIN)/$(TARGET)_boot.hex $(BIN)/$(TARGET).bin $(BOOTHEX)

$(BIN)/$(TARGET).asm: $(BIN)/$(TARGET).elf
	@echo "Disassembling to $(BIN)/$(TARGET).asm ..."
	@$(OBJDUMP) -d $(BIN)/$(TARGET).elf > $(BIN)/$(TARGET).asm

all:	$(BIN)/$(TARGET).lst $(BIN)/$(TARGET).map $(BIN)/$(TARGET).bin $(BIN)/$(TARGET).hex $(BIN)/$(TARGET)_boot.hex $(BIN)/$(TARGET).asm size

elf:	$(BIN)/$(TARGET).elf removetemp size

bin:	$(BIN)/$(TARGET).bin removetemp size removeelf

update: bin

hex:	$(BIN)/$(TARGET).hex removetemp size removeelf

asm:	$(BIN)/$(TARGET).asm removetemp size removeelf

flash:	$(BIN)/$(TARGET)_boot.hex size removeelf
	@echo "Uploading to $(DEVICE) ..."
	@$(ISPTOOL) -U flash:w:$(BIN)/$(TARGET)_boot.hex:i

fuses:
	@echo "Writing fuses to $(DEVICE) ..."
	@$(ISPTOOL) -U lfuse:w:$(LFUSE):m -U hfuse:w:$(HFUSE):m -U efuse:w:$(EFUSE):m

clean:
	@echo "Cleaning all up ..."
	@$(CLEAN)
	@rm -f $(BIN)/$(TARGET).elf $(BIN)/$(TARGET).lst $(BIN)/$(TARGET).map $(BIN)/$(TARGET).bin $(BIN)/$(TARGET).hex $(BIN)/$(TARGET)_boot.hex $(BIN)/$(TARGET).asm

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
