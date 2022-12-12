# ===================================================================================
# Project:  TapeDump64 - makefile
# Author:   Stefan Wagner
# Year:     2021
# URL:      https://github.com/wagiminator    
# ===================================================================================
# Type "make help" in the command line.
# ===================================================================================

# Input and Output File Names
SKETCH  = TapeDump64.ino
TARGET  = tapedump64

# Microcontroller Settings
DEVICE ?= attiny402
CLOCK   = 16000000
FUSE0   = 0x00
FUSE1   = 0x00
FUSE2   = 0x01
FUSE4   = 0x00
FUSE5   = 0xC5
FUSE6   = 0x04
FUSE7   = 0x00
FUSE8   = 0x00

# Toolchain
CC       = avr-gcc
OBJCOPY  = avr-objcopy
OBJDUMP  = avr-objdump
AVRSIZE  = avr-size
PYPROG   = python3 -u ./tools/tinyupdi/tinyupdi.py -d $(DEVICE)
CLEAN    = rm -f *.lst *.obj *.cof *.list *.map *.eep.hex *.o *.s *.d

# Compiler Flags
CFLAGS   = -flto -Wall -Os -mmcu=$(DEVICE) -DF_CPU=$(CLOCK) -x c++
CFLAGS  += -B include/dev/$(DEVICE) -I include

# Symbolic Targets
help:
	@echo "Use the following commands:"
	@echo "make all       compile and build $(TARGET).elf/.bin/.hex/.asm for $(DEVICE)"
	@echo "make hex       compile and build $(TARGET).hex for $(DEVICE)"
	@echo "make asm       compile and disassemble to $(TARGET).asm for $(DEVICE)"
	@echo "make bin       compile and build $(TARGET).bin for $(DEVICE)"
	@echo "make upload    compile and upload to $(DEVICE)"
	@echo "make fuses     burn fuses of $(DEVICE)"
	@echo "make install   compile, upload and burn fuses for $(DEVICE)"
	@echo "make clean     remove all build files"

all:	buildelf buildbin buildhex buildasm removetemp size

elf:	buildelf removetemp size

bin:	buildelf buildbin removetemp size removeelf

hex:	buildelf buildhex removetemp size removeelf

asm:	buildelf buildasm removetemp size removeelf

flash:	install

install:	bin
	@echo "Installing to $(DEVICE) ..."
	@$(PYPROG) --fuses 0:$(FUSE0) 1:$(FUSE1) 2:$(FUSE2) 4:$(FUSE4) 5:$(FUSE5) 6:$(FUSE6) 7:$(FUSE7) 8:$(FUSE8) --flash $(TARGET).bin

upload:	bin
	@echo "Uploading to $(DEVICE) ..."
	@$(PYPROG) --flash $(TARGET).bin

fuses:
	@echo "Burning fuses of $(DEVICE) ..."
	@$(PYPROG) --fuses 0:$(FUSE0) 1:$(FUSE1) 2:$(FUSE2) 4:$(FUSE4) 5:$(FUSE5) 6:$(FUSE6) 7:$(FUSE7) 8:$(FUSE8)

clean:
	@echo "Cleaning all up ..."
	@$(CLEAN)
	@rm -f $(TARGET).elf $(TARGET).bin $(TARGET).hex $(TARGET).asm

buildelf:
	@echo "Compiling $(SKETCH) for $(DEVICE) @ $(CLOCK)Hz ..."
	@$(CC) $(CFLAGS) $(SKETCH) -o $(TARGET).elf

buildbin:
	@echo "Building $(TARGET).bin ..."
	@$(OBJCOPY) -O binary -R .eeprom $(TARGET).elf $(TARGET).bin

buildhex:
	@echo "Building $(TARGET).hex ..."
	@$(OBJCOPY) -j .text -j .data -O ihex $(TARGET).elf $(TARGET).hex

buildasm:
	@echo "Disassembling to $(TARGET).asm ..."
	@$(OBJDUMP) -d $(TARGET).elf > $(TARGET).asm

size:
	@echo "------------------"
	@echo "FLASH: $(shell $(AVRSIZE) -d $(TARGET).elf | awk '/[0-9]/ {print $$1 + $$2}') bytes"
	@echo "SRAM:  $(shell $(AVRSIZE) -d $(TARGET).elf | awk '/[0-9]/ {print $$2 + $$3}') bytes"
	@echo "------------------"

removetemp:
	@echo "Removing temporary files ..."
	@$(CLEAN)

removeelf:
	@echo "Removing $(TARGET).elf ..."
	@rm -f $(TARGET).elf
