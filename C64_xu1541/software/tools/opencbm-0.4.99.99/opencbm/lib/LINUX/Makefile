RELATIVEPATH=../
include ${RELATIVEPATH}LINUX/config.make

CFLAGS += -I ./ -I ../libmisc/

.PHONY: all clean mrproper install uninstall install-files

LIBARCH    = ../arch/linux/
LIBMISC    = ../libmisc

# specify lib
LIBNAME = libopencbm
SRCS    = cbm.c detect.c detectxp1541.c petscii.c gcr_4b5b.c upload.c \
	  LINUX/configuration_name.c

LIBS = $(LIBARCH)/libarch.a $(LIBMISC)/libmisc.a
ifneq "$(OS)" "FreeBSD"
LIBS += -ldl
endif

all: build-lib

clean: clean-lib

mrproper: clean

install-files: install-lib

install: install-files update-libcache

uninstall: uninstall-lib
	rm -f $(DESTDIR)$(BINDIR)/opencbm_plugin_helper_tools

include ../LINUX/librules.make

### dependencies:

detect.o detect.lo: detect.c ../include/opencbm.h
detectxp1541.o detectxp1541.lo: detectxp1541.c ../include/opencbm.h
petscii.o petscii.lo: petscii.c ../include/opencbm.h
gcr_4b5b.o gcr_4b5b.lo: gcr_4b5b.c ../include/opencbm.h
upload.o upload.lo: upload.c ../include/opencbm.h
cbm.o cbm.lo: cbm.c ../include/opencbm.h ../include/LINUX/cbm_module.h
