#
# Build the firmware for the xum1541
# Copyright (c) 2009-2010 Nate Lawson <nate@root.org>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version
# 2 of the License, or (at your option) any later version.
#

# You must list all models of xum1541 here. Each model indicates the CPU
# and board type (i.e., IO port/pin layout, peripherals, etc.)
#
# Currently, valid models are:
# USBKEY: Atmel USB key development board with 8 MHz AT90USB1287 cpu
# ZOOMFLOPPY: commercial board with 16 MHz ATmega32U2 cpu
# BUMBLEB: Bumble-B with 16 MHz AT90USB162 cpu and IEC daughtercard
# OLIMEX: Olimex dev board with 8 MHz AT90USB162 cpu and IEC daughtercard
MODELS= USBKEY ZOOMFLOPPY BUMBLEB OLIMEX
.PHONY: $(MODELS)

# Enable to get debug printing via the UART (port D)
#CFLAGS=    -DDEBUG -DDEBUG_LEVEL=DBG_INFO

### Nothing user-configurable beyond this point ###

# Firmware version. Bump when changing the firmware code.
XUMFW_VERSION= 07

all: $(MODELS)

define MAKE_model
$1:
	$(MAKE) MODEL=$1 build
endef

$(foreach model,$(MODELS),$(eval $(call MAKE_model,$(model))))

ifeq ($(MODEL),USBKEY)
CPUMODEL=	at90usb1287
CPURATE=	8000000
BOARD=		BOARD_USBKEY
BOARD_OBJS=	board-usbkey.o
else ifeq ($(MODEL),ZOOMFLOPPY)
CPUMODEL=	atmega32u2
CPURATE=	16000000
BOARD=		BOARD_ZOOMFLOPPY
BOARD_OBJS=	board-zoomfloppy.o ieee.o tape_153x.o
else ifeq ($(MODEL),BUMBLEB)
CPUMODEL=	at90usb162
CPURATE=	16000000
BOARD=		BOARD_ZOOMFLOPPY
BOARD_OBJS=	board-zoomfloppy.o
else ifeq ($(MODEL),OLIMEX)
CPUMODEL=	at90usb162
CPURATE=	8000000
BOARD=		BOARD_ZOOMFLOPPY
BOARD_OBJS=	board-zoomfloppy.o
endif

# Final name for firmware hex file
MODELVERSION= xum1541-$(MODEL)-v$(XUMFW_VERSION)

# Resulting full revision number for model and firmware version
REVISION=$(shell grep '\#define $(MODEL)' xum1541.h | sed 's/\#define $(MODEL)//' | xargs printf "%02X")$(XUMFW_VERSION)

# If we wanted to reduce inlining to save size, these flags might be a
# good start:
#   --param inline-call-cost=2 -finline-limit=3 -fno-inline-small-functions
CFLAGS+= -DMODEL=$(MODEL) -DMODELNAME=\"$(MODEL)\" -DBOARD=$(BOARD) \
        -DF_CPU=$(CPURATE)UL -DF_CLOCK=F_CPU -mmcu=$(CPUMODEL) \
        -Os -g -Werror -Wall -Wstrict-prototypes -Wundef \
        -Winline -Wno-error=inline -std=gnu99 -I . \
        -DUSB_DEVICE_ONLY -DFIXED_CONTROL_ENDPOINT_SIZE=8 \
        -DINTERRUPT_CONTROL_ENDPOINT \
        -DUSE_SINGLE_DEVICE_CONFIGURATION \
        -DUSE_STATIC_OPTIONS="(USB_DEVICE_OPT_FULLSPEED | USB_OPT_REG_ENABLED | USB_OPT_AUTO_PLL)" \
        -funsigned-char -funsigned-bitfields \
        -ffunction-sections -fdata-sections \
        -fpack-struct -fshort-enums -pipe

# --gc-sections: We store each function and each variable in its 
# own section so that the linker can do dead-code/data elimination.
# Used in combo with -ffunction-sections -fdata-sections (above).
#
# --relax: use relative calls when functions are nearby
LDFLAGS+= -Wl,--gc-sections -Wl,--relax

MYUSB_OBJS= \
        LUFA/Drivers/USB/LowLevel/DevChapter9.o \
        LUFA/Drivers/USB/LowLevel/Endpoint.o \
        LUFA/Drivers/USB/LowLevel/LowLevel.o \
        LUFA/Drivers/USB/HighLevel/Events.o \
        LUFA/Drivers/USB/HighLevel/USBTask.o \
        LUFA/Drivers/USB/HighLevel/USBInterrupt.o

IEC_OBJS= iec.o s1.o s2.o pp.o p2.o nib.o

OBJS=   $(addprefix obj/$(MODEL)/,              \
        main.o commands.o descriptor.o          \
        $(BOARD_OBJS) $(MYUSB_OBJS) $(IEC_OBJS))

CC=     avr-gcc
OBJCOPY=avr-objcopy
AVRSIZE=avr-size

obj/$(MODEL)/%.o: %.c
	mkdir -p $(dir $@)
	${CC} $(CFLAGS) -c -o $@ $< 

build: $(OBJS)
	${CC} $(CFLAGS) -o obj/$(MODEL)/$(MODELVERSION).bin $(OBJS) $(LDFLAGS)
	${AVRSIZE}         obj/$(MODEL)/$(MODELVERSION).bin
	${OBJCOPY} -O ihex obj/$(MODEL)/$(MODELVERSION).bin $(MODELVERSION).hex
	sed -e 's/XUM1541 USB floppy adapter [(]generic[)]/XUM1541 USB floppy adapter ($(MODEL)-v$(XUMFW_VERSION))/' \
	    -e 's/ VID_16D0&PID_0504$$/ VID_16d0\&PID_0504\&REV_$(REVISION)/' \
	    < xum1541-generic.inf > $(MODELVERSION).inf

clean:
	rm -rf -- obj xum1541-*-v$(XUMFW_VERSION).inf


mrproper: clean
	rm -f -- *~ */*~
