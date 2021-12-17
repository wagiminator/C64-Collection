#
# Makefile
#

# call with "export XMINGW=mingw" before if you want to use the MINGW cross compiler

ifeq "1" "2"

ifneq "$(MINGW)" ""
 # we are using the MINGW cross compiler

 #XMINGW_ROOT = /usr/local/cross-tools/bin
 #CC = $(XMINGW_ROOT)/i386-mingw32msvc-gcc
 CC = /usr/bin/i586-mingw32msvc-gcc
 LIBUSB_DIR = ~/LibUSB

 LDFLAGS_EXTRA=-L $(LIBUSB_DIR)/lib/gcc/ -llibusb.lib
 CFLAGS_EXTRA = -DWIN
 EXE_SUFFIX = .exe

else

 CFLAGS_EXTRA =
 # determine libusb location
 OS=$(shell uname -s)
 ifeq "$(OS)" "Darwin"
   # MacOS compilation:

   LIBUSB_DIR = $(HOME)/libusb
 else
   ifeq "$(OS)" "Cygwin"
     # Cygwin compilation

     # Tell, where your libusb-win32 installation resides
     # (if not part of Cygwin in the latest version)
     #
     # LIBUSB_DIR=/cygdrive/c/drivers/LibUSB
     LIBUSB_DIR=/cygdrive/n/Programme/LibUSB

     LDFLAGS_EXTRA=-L $(LIBUSB_DIR)/lib/gcc

     # For compiling for MinGw32 target
     #
     CFLAGS_EXTRA=-mno-cygwin
   else
     LIBUSB_DIR = /usr
   endif
 endif
endif

endif

include ../include/common.mk

# remove if you want to generate debugging information
LDFLAGS_EXTRA+=-s

.PHONY: all clean mrproper xu1541lib

# add libusb
CFLAGS  = $(CFLAGS_EXTRA) -I$(LIBUSB_DIR)/include -I../include/ -Wall
LDFLAGS = $(LDFLAGS_EXTRA) -L$(LIBUSB_DIR)/lib
LIBS    = -L../lib -lxu1541$(LIB_WIN) -lusb

ECHO_TEST = usb_echo_test
EVENT_LOG = read_event_log

all: $(ECHO_TEST)$(EXE_SUFFIX) $(EVENT_LOG)$(EXE_SUFFIX)

clean:
	rm -f $(ECHO_TEST)$(EXE_SUFFIX) $(EVENT_LOG)$(EXE_SUFFIX)

mrproper: clean
	rm -f $(ECHO_TEST) $(EVENT_LOG) $(ECHO_TEST).exe $(EVENT_LOG).exe

%: %.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LIBS)

%.exe: %.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LIBS)

$(ECHO_TEST)$(EXE_SUFFIX): xu1541lib

$(EVENT_LOG)$(EXE_SUFFIX): xu1541lib

xu1541lib:
	make -C ../lib/
