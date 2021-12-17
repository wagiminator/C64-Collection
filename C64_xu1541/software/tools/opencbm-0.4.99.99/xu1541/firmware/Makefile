# Name: Makefile
# Project: XU1541
# Author: Christian Starkjohann, modified for xu1541 by Till Harbaum
# Creation Date: 2005-03-20
# Tabsize: 4
# Copyright: (c) 2005 by OBJECTIVE DEVELOPMENT Software GmbH
# License: Proprietary, free under certain conditions. See Documentation.

include ../bootloader/Makefile.common
LDFLAGS += -Wl,$(COMMON_SECTIONS) -nostdlib


SERIAL = `echo /dev/tty.[Uu][Ss]*`
UISP = uisp -dprog=stk200
# UISP = uisp -dprog=avr910 -dserial=$(SERIAL) -dpart=auto
# The two lines above are for "uisp" and the AVR910 serial programmer connected
# to a Keyspan USB to serial converter to a Mac running Mac OS X.
# Choose your favorite programmer and interface.

DEFINES += -DF_CPU=12000000
COMPILE = avr-gcc -Wall -O2 --param max-inline-insns-auto=3 -I. -mmcu=atmega8 $(DEFINES) -I../include/

OBJECTS = gcrt1.o memcpy.o firmwaretable.o main.o xu1541.o s1.o s2.o pp.o p2.o event_log.o


# symbolic targets:
all:	firmware.hex

.INTERMEDIATE: $(OBJECTS)

.c.o:
	$(COMPILE) -c $< -o $@

.S.o:
	$(COMPILE) -x assembler-with-cpp -c $< -o $@
# "-x assembler-with-cpp" should not be necessary since this is the default
# file type for the .S (with capital S) extension. However, upper case
# characters are not always preserved on Windows. To ensure WinAVR
# compatibility define the file type manually.

.c.s:
	$(COMPILE) -S $< -o $@

# Fuse high byte:
# 0xc9 = 1 1 0 0   1 0 0 1 <-- BOOTRST (boot reset vector at 0x0000)
#        ^ ^ ^ ^   ^ ^ ^------ BOOTSZ0
#        | | | |   | +-------- BOOTSZ1
#        | | | |   + --------- EESAVE (don't preserve EEPROM over chip erase)
#        | | | +-------------- CKOPT (full output swing)
#        | | +---------------- SPIEN (allow serial programming)
#        | +------------------ WDTON (WDT not always on)
#        +-------------------- RSTDISBL (reset pin is enabled)
# Fuse low byte:
# 0x9f = 1 0 0 1   1 1 1 1
#        ^ ^ \ /   \--+--/
#        | |  |       +------- CKSEL 3..0 (external >8M crystal)
#        | |  +--------------- SUT 1..0 (crystal osc, BOD enabled)
#        | +------------------ BODEN (BrownOut Detector enabled)
#        +-------------------- BODLEVEL (2.7V)
fuse:
	$(UISP) --wr_fuse_h=0xc9 --wr_fuse_l=0x9f


clean:
	rm -f firmware.hex firmware.lst firmware.obj firmware.cof firmware.list firmware.map firmware.eep.hex firmware.bin *.o firmware.s

# file targets:
firmware.bin:	$(OBJECTS)
	echo	$(COMPILE) -o firmware.bin $(OBJECTS) ${LDFLAGS}
	$(COMPILE) -o firmware.bin $(OBJECTS) ${LDFLAGS}

firmware.hex:	firmware.bin
	rm -f firmware.hex firmware.eep.hex
	avr-objcopy -j .text -j .data -j .textfirmwaretable -O ihex firmware.bin firmware.hex
	./checksize firmware.bin 8192 960
# do the checksize script as our last action to allow successful compilation
# on Windows with WinAVR where the Unix commands will fail.

program: firmware.hex
	avrdude -c stk200 -p atmega8 -U lfuse:w:0x9f:m -U hfuse:w:0xc9:m -U flash:w:firmware.hex

program-usb: firmware.hex
	avrdude -c usbasp -p atmega8 -U lfuse:w:0x9f:m -U hfuse:w:0xc9:m -U flash:w:firmware.hex

program-xu: firmware.hex
	../update_tool/xu1541_update firmware.hex

cpp:
	$(COMPILE) -E main.c

disasm:	firmware.bin
	avr-objdump -d $<


