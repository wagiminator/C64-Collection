# ======================================================================
# Common Makefile for USBtiny applications
#
# Macros to be defined before including this file:
#
# USBTINY	- the location of this directory
# TARGET_ARCH	- gcc -mmcu= option with AVR device type
# OBJECTS	- the objects in addition to the USBtiny objects
# FLASH_CMD	- command to upload main.hex to flash
# FUSES_CMD	- command to program the fuse bytes
# STACK		- maximum stack size (optional)
# FLASH		- flash size (optional)
# SRAM		- SRAM size (optional)
# SCHEM		- Postscript version of the schematic to be generated
#
# Copyright 2006-2008 Dick Streefland
#
# This is free software, licensed under the terms of the GNU General
# Public License as published by the Free Software Foundation.
# ======================================================================

CC	= avr-gcc
CFLAGS	= -Os -g -Wall -I. -I$(USBTINY) -I$(XU1541_INCLUDE)
ASFLAGS	= -Os -g -Wall -I.
LDFLAGS	+= -g
MODULES = crc.o int.o usb.o $(OBJECTS)
UTIL	= $(USBTINY)/..

bootldr-usbtiny.hex:

all:		bootldr-usbtiny.hex $(SCHEM)

clean:
	rm -f bootldr-usbtiny.bin *.o tags *.sch~ gschem.log *~

clobber:	clean
	rm -f bootldr-usbtiny.hex $(SCHEM)

bootldr-usbtiny.bin:	$(MODULES)
	$(LINK.o) -o $@ $(MODULES) $(LDFLAGS)

bootldr-usbtiny.hex:	bootldr-usbtiny.bin $(UTIL)/check.py
	@python $(UTIL)/check.py bootldr-usbtiny.bin $(STACK) $(FLASH) $(SRAM)
	avr-objcopy -j .text -j .data -j .textbiostable -j .textadd -O ihex bootldr-usbtiny.bin bootldr-usbtiny.hex

disasm:		bootldr-usbtiny.bin
	avr-objdump -S bootldr-usbtiny.bin

program:	flash

flash:		bootldr-usbtiny.hex
	$(FLASH_CMD)

fuses:
	$(FUSES_CMD)

crc.o:		$(USBTINY)/crc.S $(USBTINY)/def.h usbtiny.h
	$(COMPILE.c) $(USBTINY)/crc.S
int.o:		$(USBTINY)/int.S $(USBTINY)/def.h usbtiny.h
	$(COMPILE.c) $(USBTINY)/int.S
usb.o:		$(USBTINY)/usb.c $(USBTINY)/def.h $(USBTINY)/usb.h usbtiny.h
	$(COMPILE.c) $(USBTINY)/usb.c

bootldr-usbtiny.o:		$(USBTINY)/usb.h

%.ps:		%.sch $(UTIL)/sch2ps
	$(UTIL)/sch2ps $<
