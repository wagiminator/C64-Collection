RELATIVEPATH=../../../
include ${RELATIVEPATH}LINUX/config.make

.PHONY: all clean mrproper install uninstall install-files

PLUGIN_NAME = xa1541
LIBNAME = libopencbm-${PLUGIN_NAME}
SRCS    = LINUX/iec.c LINUX/parburst.c

CFLAGS += -I../../../include/LINUX/ -I../../../include/ -I../../

all: build-lib

clean: clean-lib

mrproper: clean

install-files: install-plugin

install: install-files

uninstall: uninstall-plugin

include ../../../LINUX/librules.make

### dependencies:

LINUX/iec.o LINUX/iec.lo: LINUX/iec.c ../../archlib.h
LINUX/parburst.o LINUX/parburst.lo: LINUX/parburst.c ../../archlib.h
