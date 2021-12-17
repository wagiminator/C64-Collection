#
# Makefile
#

# call with "export XMINGW=mingw" before if you want to use the MINGW cross compiler

LIB_SUFFIX=.a
LIB_WIN=
AR=ar
CYGWIN=

ifneq "$(MINGW)" ""
 # we are using the MINGW cross compiler

 #XMINGW_ROOT = /usr/local/cross-tools/bin/i386-mingw32msvc-
 XMINGW_ROOT = /usr/bin/i586-mingw32msvc-
 CC = $(XMINGW_ROOT)gcc
 AR = $(XMINGW_ROOT)ar

 LIBUSB_DIR = $(HOME)/LibUSB

 LDFLAGS_EXTRA=-L $(LIBUSB_DIR)/lib/gcc/
 CFLAGS_EXTRA = -DWIN
 EXE_SUFFIX = .exe
 LIB_WIN = .lib

else

 CFLAGS_EXTRA =
 # determine libusb location
 OS=$(shell uname -s)
 ifeq "$(OS)" "Darwin"
   # MacOS compilation:

   LIBUSB_DIR = $(shell libusb-legacy-config --prefix)
   LDFLAGS_EXTRA = $(shell libusb-legacy-config --libs)
   CFLAGS_EXTRA = $(shell libusb-legacy-config --cflags)
   
 else
   ifeq "$(shell uname -o)" "Cygwin"
     # Cygwin compilation

     # Tell, where your libusb-win32 installation resides
     # (if not part of Cygwin in the latest version)
     #
     # LIBUSB_DIR=/cygdrive/c/drivers/LibUSB
#     LIBUSB_DIR=/cygdrive/n/Programme/LibUSB
     LIBUSB_DIR=$(HOME)/LibUSB
     LIB_WIN = .lib
     EXE_SUFFIX = .exe

     LDFLAGS_EXTRA=-mno-cygwin -L $(LIBUSB_DIR)/lib/gcc
     CYGWIN=1

     # For compiling for MinGw32 target
     #
     CFLAGS_EXTRA=-mno-cygwin -DWIN
   else
     LIBUSB_DIR = /usr
   endif
 endif
endif

CFLAGS_EXTRA+=-I$(LIBUSB_DIR)/include/
