ifneq ($(KERNELRELEASE),)
EXTRA_CFLAGS := $(CBM4LINUX_KERNEL_FLAGS) -I$(here)/../../include -I$(here)/../../include/LINUX
obj-m 	     := cbm.o
cbm-objs     := cbm_module.o

# for 2.2/2.4:
modules: checksources
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -c -o $(obj-m) cbm_module.c

else

RELATIVEPATH=../../
include ${RELATIVEPATH}LINUX/config.make

all: cbm.o

.PHONY: all clean mrproper install uninstall install-files checksources
clean:
	rm -f cbm.mod.c *.o *.ko .cbm* Modules.symvers
	rm -fr .tmp_versions
	rm -f Makefile
	rm -f Module.symvers modules.order

mrproper: clean

install-files: all
ifneq "$(MODDIR)" ""
	install -m 644 `[ -f cbm.ko ] && echo cbm.ko || echo cbm.o` $(DESTDIR)$(MODDIR)
endif
ifneq ($(UDEV_RULES),)
	install -d -m 755 $(DESTDIR)$(UDEV_RULES)
	install -m 644 45-opencbm-xa1541.rules $(DESTDIR)$(UDEV_RULES)
endif

install: install-files
	-depmod -a

uninstall:
	if [ ! -z "`lsmod|grep '^cbm '`" ]; then rmmod cbm; fi
ifneq "$(MODDIR)" ""
	rm -f $(DESTDIR)$(MODDIR)/cbm.o $(DESTDIR)$(MODDIR)/cbm.ko
endif
ifneq ($(UDEV_RULES),)
	rm -f $(DESTDIR)$(UDEV_RULES)/45-opencbm-xa1541.rules
endif

cbm.o: checksources cbm_module.c
	-ln -s LINUX/Makefile Makefile
	$(MAKE) -C $(KERNEL_SOURCE) here=`pwd` CBM4LINUX_KERNEL_FLAGS=$(KERNEL_FLAGS) SUBDIRS=`pwd` modules
	-rm -f Makefile

endif

checksources:
	# check if KERNEL_SOURCE is set
	if [ -z "${KERNEL_SOURCE}" ]; then echo "No KERNEL_SOURCE found, aborting!"; exit 1; fi


