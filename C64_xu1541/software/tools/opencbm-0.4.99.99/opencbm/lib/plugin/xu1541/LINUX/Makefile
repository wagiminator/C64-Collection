RELATIVEPATH=../../../
include ${RELATIVEPATH}LINUX/config.make

.PHONY: all clean mrproper install uninstall install-files

PLUGIN_NAME = xu1541
LIBNAME = libopencbm-${PLUGIN_NAME}
SRCS    = archlib.c xu1541.c s1_s2_pp.c
LIBS    = -L$(RELATIVEPATH)/libmisc -lmisc
LIBS   += $(LIBUSB_LIBS)

CFLAGS += $(LIBUSB_CFLAGS)
CFLAGS += -I$(RELATIVEPATH)/include/LINUX/ -I$(RELATIVEPATH)/include/ -I../../ -I$(XU1541DIR)/include -I$(RELATIVEPATH)/libmisc
#LDFLAGS +=

all: build-lib

clean: clean-lib

mrproper: clean

install-files: install-plugin

install: install-files

uninstall: uninstall-plugin

include ../../../LINUX/librules.make

### dependencies:

archlib.o archlib.lo: ../../archlib.h xu1541.h
xu1541.o xu1541.lo: xu1541.h
