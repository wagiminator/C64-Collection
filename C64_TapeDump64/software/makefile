# Project:  TapeDump64
# Author:   Stefan Wagner
# Year:     2021
# URL:      https://github.com/wagiminator    
#
# Download AVR 8-bit Toolchain:
# https://www.microchip.com/mplab/avr-support/avr-and-arm-toolchains-c-compilers
# and extract to ./tools/avr-gcc 
# Type "make help" in the command line.

# Input and Output File Names
SKETCH  = TapeDump64.ino
TARGET  = tapedump64

# Microcontroller Options
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

# Programmer Options
PROGRMR = serialupdi
PORT   ?= /dev/ttyUSB0

# Paths
GCCPATH = ./tools/avr-gcc
DFPPATH = ./tools/dfp
PYMPATH = ./tools/pymcuprog
ADCPATH = ./tools/avrdude

# Commands
DFPINCL = -B $(DFPPATH)/gcc/dev/$(DEVICE)/ -I $(DFPPATH)/include/
COMPILE = $(GCCPATH)/bin/avr-gcc $(DFPINCL) -flto -Wall -Os -mmcu=$(DEVICE) -DF_CPU=$(CLOCK)UL -x c++ $(SKETCH)
PYPROG  = python -u $(PYMPATH)/prog.py -t uart -u $(PORT) -b 230400 -d $(DEVICE)
CLEAN   = rm -f *.lst *.obj *.cof *.list *.map *.eep.hex *.o *.s *.d

# Symbolic Targets
help:
	@echo "Use the following commands:"
	@echo "make all       compile and build $(TARGET).bin/.hex/.asm for $(DEVICE)"
	@echo "make hex       compile and build $(TARGET).hex for $(DEVICE)"
	@echo "make asm       compile and disassemble to $(TARGET).asm for $(DEVICE)"
	@echo "make bin       compile and build $(TARGET).bin for $(DEVICE)"
	@echo "make upload    compile and upload to $(DEVICE) using $(PROGRMR)"
	@echo "make fuses     burn fuses of $(DEVICE) using $(PROGRMR) programmer"
	@echo "make install   compile, upload and burn fuses for $(DEVICE)"
	@echo "make clean     remove all build files"

all:	buildbin buildhex buildasm removetemp size

bin:  buildbin removetemp size

hex:	buildbin buildhex removetemp size removebin

asm:	buildbin buildasm removetemp size removebin

install: fuses upload

upload:	hex
	@echo "Uploading to $(DEVICE) ..."
	@$(PYPROG) --fuses 2:$(FUSE2) 6:$(FUSE6) 8:$(FUSE8) -f $(TARGET).hex -a write

fuses:
	@echo "Burning fuses of $(DEVICE) ..."
	@$(PYPROG) --fuses 0:$(FUSE0) 1:$(FUSE1) 2:$(FUSE2) 4:$(FUSE4) 5:$(FUSE5) 6:$(FUSE6) 7:$(FUSE7) 8:$(FUSE8) -a erase

clean:
	@echo "Cleaning all up ..."
	@$(CLEAN)
	@rm -f $(TARGET).bin $(TARGET).hex $(TARGET).asm

buildbin:
	@echo "Building $(TARGET).bin for $(DEVICE) @ $(CLOCK)Hz ..."
	@$(COMPILE) -o $(TARGET).bin

buildhex:
	@echo "Building $(TARGET).hex ..."
	@$(GCCPATH)/bin/avr-objcopy -O ihex -R .eeprom $(TARGET).bin $(TARGET).hex

buildasm:
	@echo "Disassembling to $(TARGET).asm ..."
	@$(GCCPATH)/bin/avr-objdump -d $(TARGET).bin > $(TARGET).asm

size:
	@echo "FLASH: $(shell $(GCCPATH)/bin/avr-size -d $(TARGET).bin | awk '/[0-9]/ {print $$1 + $$2}') bytes"
	@echo "SRAM:  $(shell $(GCCPATH)/bin/avr-size -d $(TARGET).bin | awk '/[0-9]/ {print $$2 + $$3}') bytes"

removetemp:
	@echo "Removing temporary files ..."
	@$(CLEAN)

removebin:
	@echo "Removing $(TARGET).bin ..."
	@rm -f $(TARGET).bin
