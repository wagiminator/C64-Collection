# get package version (major.minor.release).

VERSIONH:=$(RELATIVEPATH)include/version.h

OPENCBM_MAJOR   :=$(shell grep "\#define OPENCBM_VERSION_MAJOR"      ${VERSIONH}|sed -e "s/\#define OPENCBM_VERSION_[^ ]*[ ]*//")
OPENCBM_MINOR   :=$(shell grep "\#define OPENCBM_VERSION_MINOR"      ${VERSIONH}|sed -e "s/\#define OPENCBM_VERSION_[^ ]*[ ]*//")
OPENCBM_RELEASE :=$(shell grep "\#define OPENCBM_VERSION_SUBMINOR"   ${VERSIONH}|sed -e "s/\#define OPENCBM_VERSION_[^ ]*[ ]*//")
OPENCBM_PATCHLVL:=$(shell grep "\#define OPENCBM_VERSION_PATCHLEVEL" ${VERSIONH}|sed -e "s/\#define OPENCBM_VERSION_[^ ]*[ ]*//")
OPENCBM_DEVEL   :=$(shell grep "\#define OPENCBM_VERSION_DEVEL"      ${VERSIONH}|sed -e "s/\#define OPENCBM_VERSION_[^ ]*[ ]*//")

MAJ:=${OPENCBM_MAJOR}
MIN:=${OPENCBM_MINOR}

ifeq ($(OPENCBM_DEVEL), 0)
	REL:=${OPENCBM_RELEASE}
else
	REL:=${OPENCBM_RELEASE}.${OPENCBM_DEVEL}
endif

# choose your crossassembler (if you have one).
# mandatory if you want to hack any of the 6502 sources.
#
#XASS        = xa
XASS        = cl65


#
# Default destination directories
#
PREFIX      = /usr/local
ETCDIR      = $(PREFIX)/etc
BINDIR      = $(PREFIX)/bin
LIBDIR      = $(PREFIX)/lib
MANDIR      = $(PREFIX)/man/man1
INFODIR     = $(PREFIX)/info
INCDIR      = $(PREFIX)/include
MODDIR      = ${shell for d in /lib/modules/`uname -r`/extra /lib/modules/`uname -r`/misc /lib/modules/`uname -r`/kernel/drivers/char; do test -d $$d && echo $$d; done | head -n 1}
PLUGINDIR   = $(PREFIX)/lib/opencbm/plugin/
UDEV_RULES  = /etc/udev/rules.d/

#
# Where to find the xum1541 and xu1541 firmware
#
XU1541DIR   = $(RELATIVEPATH)/../xu1541
XUM1541DIR  = $(RELATIVEPATH)/../xum1541

#
# Where to find libusb (libusb.sf.net)
#
LIBUSB_CFLAGS  = -I/usr/include
LIBUSB_LDFLAGS =
LIBUSB_LIBS    = -L/usr/lib -lusb

#
# define os name
#
OS = $(shell uname -s)

#
# compiler/linker flags. Should be ok.
#
OS_ARCH     = linux

CFLAGS       = -O2 -Wall -I../include -I../include/LINUX -DPREFIX=\"$(PREFIX)\" -DOPENCBM_CONFIG_FILE=\"$(OPENCBM_CONFIG_FILE)\"
CFLAGS      += $(USER_CFLAGS)

LIB_CFLAGS   = $(CFLAGS) -D_REENTRANT
SHLIB_CFLAGS = $(LIB_CFLAGS) -fPIC
SHLIB_EXT    = so
SHLIB_SWITCH = -shared
LINK_FLAGS   = -L../lib -L../arch/$(OS_ARCH) -L../libmisc -lopencbm -larch -lmisc
SONAME       = -Wl,-soname -Wl,
CC           = gcc
AR           = ar
LDCONFIG     = /sbin/ldconfig
OD_FLAGS     = -w8 -txC -v -An

#
# location of the kernel source directory
# (removed, use the later implementation instead. I left them in in case the
#  later implementation does not work on a particular machine.)
#KERNEL_SOURCE = /usr/src/linux                      # for kernel 2.4
#KERNEL_SOURCE = /lib/modules/`uname -r`/build       # for kernel 2.6

# from patch #1189489 on SourceForge, with fix from #1189492):
KERNEL_SOURCE = ${shell for d in /lib/modules/`uname -r`/build /usr/src/linux; do test -e $$d && echo $$d; done | head -n 1}

#
# Find out if we should include linux/autoconf.h or linux/conf.h in the kernel module
#
KERNEL_INCLUDE_CONFIG = ${shell for c in ${KERNEL_SOURCE}/include/linux/autoconf.h ${KERNEL_SOURCE}/include/linux/config.h; do test -f $$c && echo $$c; done | head -n 1}

#
# kernel driver compile flags.
#
# add `-DDIRECT_PORT_ACCESS' to avoid usage of the generic parport module
#   (pre-0.3.0 behaviour, definitely needed for kernel 2.0.x)
#
# add `-DOLD_C4L_CABLE' if you want to use your old (cbm4linux <= 0.2.0)
#   XE1541-like cable. Don't to it. Upgrade to XM1541 instead.
#
#KERNEL_FLAGS = -DDIRECT_PORT_ACCESS
KERNEL_FLAGS = ${KERNEL_DEFINE}


#
# Linux specific settings and modifications
#
ifeq "$(OS)" "Linux"
ETCDIR=/etc
endif

#
# FreeBSD specific settings and modifications
#
ifeq "$(OS)" "FreeBSD"
ETCDIR=$(PREFIX)/etc
OD_FLAGS  = -txC -v -An
endif

#
# MacOS X (Darwin) specific settings and modifications
#
ifeq "$(OS)" "Darwin"
ETCDIR=$(PREFIX)/etc

# Use MacPort's libusb-legacy for now
LIBUSB_CONFIG  = /opt/local/bin/libusb-legacy-config
LIBUSB_CFLAGS  = $(shell $(LIBUSB_CONFIG) --cflags)
LIBUSB_LDFLAGS =
LIBUSB_LIBS    = $(shell $(LIBUSB_CONFIG) --libs)

OD_FLAGS  = -txC -v -An
SHLIB_EXT = dylib
SHLIB_SWITCH = -dynamiclib -compatibility_version $(MAJ).$(MIN) -current_version $(MAJ).$(MIN).${OPENCBM_RELEASE}
SHLIB_CFLAGS += -fno-common
SONAME = -install_name $(PREFIX)/lib/
endif

#
#
#

OPENCBM_CONFIG_PATH = $(ETCDIR)

OPENCBM_CONFIG_FILE = $(OPENCBM_CONFIG_PATH)/opencbm.conf

#
# common compile flags
#
.SUFFIXES: .a65 .o65 .inc .lo

.c.lo:
	$(CC) $(SHLIB_CFLAGS) -c -o $@ $<

.o65.inc:
	test -s $< && od $(OD_FLAGS) $< | \
	sed 's/\([0-9a-f]\{2\}\) */0x\1,/g; $$s/,$$//' > $@


#
# cross assembler definitions.
# patches for other assemblers are welcome.
#

# xa defs
XA          = xa

# cl65 defs, contributed by Ullrich von Bassewitz <uz(at)musoftware(dot)de>
# (cc65 >= 2.6 required)
CL65        = cl65
LD65        = ld65
CA65_FLAGS  = --feature labels_without_colons --feature pc_assignment --feature loose_char_term --asm-include-dir ..


#
# suffix rules
#
.a65.o65:
ifeq ($(XASS),xa)
	$(XA) $< -o $@
else
ifeq ($(XASS),cl65)
	$(CL65) -c $(CA65_FLAGS) -o $*.tmp $<
	$(LD65) -o $@ --target none $*.tmp && rm -f $*.tmp
else
	@echo "*** Error: No crossassembler defined. Check config.make" 2>&1
	exit 1
endif
endif
