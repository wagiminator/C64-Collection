RELATIVEPATH=opencbm/
include ${RELATIVEPATH}LINUX/config.make

CBMDEV   = /dev/cbm
CBMPERM  = 666
DEVMAJOR = 10
DEVMINOR = 177
SUBDIRS  = opencbm/include opencbm/arch/$(OS_ARCH) opencbm/libmisc opencbm/lib \
	   opencbm/libtrans \
           opencbm/cbmctrl opencbm/cbmformat opencbm/cbmforng opencbm/d64copy opencbm/cbmcopy \
	   opencbm/d82copy opencbm/imgcopy \
           opencbm/demo/flash opencbm/demo/morse opencbm/demo/rpm1541 \
	   opencbm/sample/libtrans
ifeq "$(OS)" "Linux"
SUBDIRS += opencbm/compat
endif

SUBDIRS_DOC = opencbm/docs

SUBDIRS_PLUGIN_XU1541 = opencbm/lib/plugin/xu1541

SUBDIRS_PLUGIN_XUM1541 = opencbm/lib/plugin/xum1541 xum1541cfg

SUBDIRS_PLUGIN_XA1541 = opencbm/lib/plugin/xa1541 opencbm/sys/linux/

SUBDIRS_OPTIONAL = opencbm/addon opencbm/nibtools opencbm/mnib36 opencbm/cbmrpm41 opencbm/cbmlinetester


SUBDIRS_PLUGIN          = $(SUBDIRS_PLUGIN_XUM1541) $(SUBDIRS_PLUGIN_XU1541) $(SUBDIRS_PLUGIN_XA1541)

SUBDIRS_ALL_NON_OPTIONAL= $(SUBDIRS) $(SUBDIRS_DOC) $(SUBDIRS_PLUGIN)

ifeq "$(OS)" "Darwin"
PLUGINS=plugin-xum1541 plugin-xu1541
INSTALL_PLUGINS=install-plugin-xum1541 install-plugin-xu1541
else
ifeq "$(OS)" "FreeBSD"
PLUGINS=plugin-xum1541 plugin-xu1541
INSTALL_PLUGINS=install-plugin-xum1541 install-plugin-xu1541
else
PLUGINS=plugin-xum1541 plugin-xu1541 plugin-xa1541
INSTALL_PLUGINS=install-plugin-xum1541 install-plugin-xu1541 install-plugin-xa1541
endif
endif

.PHONY: all opencbm clean mrproper dist doc install-all install install-doc uninstall dev install-files install-files-doc all-doc plugin-xum1541 plugin-xu1541 plugin-xa1541 plugin install-plugin install-plugin-xum1541 install-plugin-xu1541 install-plugin-xa1541

CREATE_TARGET = $(patsubst %,BUILDSYSTEM.%,$(1:=.$2))
CREATE_TARGETS = $(patsubst %,BUILDSYSTEM.%,$(foreach base, $2, $(1:=.$(base))))
GET_TARGET_DIR = $(patsubst BUILDSYSTEM.%,%,$(basename $@))
GET_TARGET =$(patsubst .%,%,$(suffix $@))


all: opencbm plugin

all-doc: all doc

.PHONY: $(call CREATE_TARGETS,$(SUBDIRS_ALL_NON_OPTIONAL),all clean mrproper install install-files install-files-doc install-doc uninstall)
$(call CREATE_TARGETS,$(SUBDIRS_ALL_NON_OPTIONAL),all clean mrproper install install-files install-files-doc install-doc uninstall)::
	$(MAKE) -C $(call GET_TARGET_DIR,$@) -f LINUX/Makefile $(call GET_TARGET,$@)

.PHONY: $(call CREATE_TARGETS,$(SUBDIRS_OPTIONAL),all clean mrproper install install-files install-files-doc install-doc uninstall)
$(call CREATE_TARGETS,$(SUBDIRS_OPTIONAL),all clean mrproper install install-files install-files-doc install-doc uninstall)::
	test ! -e $(call GET_TARGET_DIR,$@)/LINUX/Makefile || $(MAKE) -C $(call GET_TARGET_DIR,$@) -f LINUX/Makefile $(call GET_TARGET,$@)

clean:  $(call CREATE_TARGETS,$(SUBDIRS_ALL_NON_OPTIONAL) $(SUBDIRS_OPTIONAL),clean)
	rm -f xu1541/misc/usb_echo_test xu1541/misc/read_event_log xu1541/lib/libxu1541.a

mrproper:  $(call CREATE_TARGETS,$(SUBDIRS_ALL_NON_OPTIONAL) $(SUBDIRS_OPTIONAL),mrproper)
	rm -f *~ LINUX/*~ WINDOWS/*~
	rm -f xu1541/misc/usb_echo_test xu1541/misc/read_event_log xu1541/lib/libxu1541.a

opencbm: $(call CREATE_TARGET,$(SUBDIRS) $(SUBDIRS_OPTIONAL),all)



doc: $(call CREATE_TARGETS,$(SUBDIRS_DOC),all)


dist: all-doc clean
	rm -f build-stamp configure-stamp
	tar czvf ../`basename \`pwd\``.tar.gz --exclude=.git --exclude=debian -C .. `basename \`pwd\``

install-files: $(call CREATE_TARGET,$(SUBDIRS) $(SUBDIRS_OPTIONAL),install-files)
$(call CREATE_TARGET,$(SUBDIRS) $(SUBDIRS_OPTIONAL),install-files):: install-files.prepare

install-files.prepare: opencbm
	mkdir -p -m 755 $(DESTDIR)$(BINDIR) $(DESTDIR)$(LIBDIR) $(DESTDIR)$(MANDIR) $(DESTDIR)$(INCDIR) $(DESTDIR)$(INFODIR)
ifeq "$(OS)" "Linux"
ifneq "$(MODDIR)" ""
	mkdir -p -m 755 $(DESTDIR)$(MODDIR)
endif
endif

install: install-files

install-all: install install-plugin

install-plugin-xu1541: $(call CREATE_TARGET,$(SUBDIRS_PLUGIN_XU1541),install)
ifeq "$(OS)" "Linux"
	install -m 644 xu1541/udev/45-opencbm-xu1541.rules ${DESTDIR}/etc/udev/rules.d/
endif
	install -d $(DESTDIR)$(PLUGINDIR)/xu1541
	install -m 755 xu1541/misc/usb_echo_test xu1541/misc/read_event_log ${DESTDIR}/${PLUGINDIR}/xu1541/

$(call CREATE_TARGET,$(SUBDIRS_PLUGIN_XU1541),install):: plugin-xu1541

install-plugin-xum1541: $(call CREATE_TARGET,$(SUBDIRS_PLUGIN_XUM1541),install)
ifeq "$(OS)" "Linux"
	install -m 644 xum1541/udev/45-opencbm-xum1541.rules ${DESTDIR}/etc/udev/rules.d/
endif

$(call CREATE_TARGET,$(SUBDIRS_PLUGIN_XUM1541),install):: plugin-xum1541

install-plugin-xa1541: $(call CREATE_TARGET,$(SUBDIRS_PLUGIN_XA1541),install)

$(call CREATE_TARGET,$(SUBDIRS_PLUGIN_XA1541),install):: plugin-xa1541


install-plugin: $(INSTALL_PLUGINS)

install-files-doc: $(call CREATE_TARGET,$(SUBDIRS_DOC),install-files-doc)
$(call CREATE_TARGET,$(SUBDIRS_DOC),install-files-doc):: install-files all-doc

install-doc: $(call CREATE_TARGET,$(SUBDIRS_DOC),install-doc)
$(call CREATE_TARGET,$(SUBDIRS_DOC),install-doc):: install-files-doc install


plugin-xu1541: $(call CREATE_TARGET,$(SUBDIRS_PLUGIN_XU1541),all)
	$(MAKE) --dir=xu1541/misc all

$(call CREATE_TARGET,$(SUBDIRS_PLUGIN_XU1541),all):: opencbm

plugin-xum1541: $(call CREATE_TARGET,$(SUBDIRS_PLUGIN_XUM1541),all)

$(call CREATE_TARGET,$(SUBDIRS_PLUGIN_XUM1541),all):: opencbm

plugin-xa1541: $(call CREATE_TARGET,$(SUBDIRS_PLUGIN_XA1541),all)

$(call CREATE_TARGET,$(SUBDIRS_PLUGIN_XA1541),all):: opencbm

plugin: $(PLUGINS)

uninstall: $(call CREATE_TARGET,$(SUBDIRS_ALL_NON_OPTIONAL) $(SUBDIRS_OPTIONAL),uninstall)
	rm ${DESTDIR}/etc/udev/rules.d/45-opencbm-xa1541.rules \
	   ${DESTDIR}/etc/udev/rules.d/45-opencbm-xu1541.rules \
	   ${DESTDIR}/etc/udev/rules.d/45-opencbm-xum1541.rules \
	   ${DESTDIR}/etc/ld.so.conf.d/99-opencbm.conf \
	   ${DESTDIR}/${PLUGINDIR}/xu1541/usb_echo_test \
	   ${DESTDIR}/${PLUGINDIR}/xu1541/read_event_log \
	|| /bin/true
	ldconfig || /bin/true

ldconfig:
	echo "${BINDIR}" > ${DESTDIR}/etc/ld.so.conf.d/99-opencbm.conf
	ldconfig

dev:
	mkdir -p -m 755 `dirname $(CBMDEV)`
	rm -f $(CBMDEV)
	mknod -m $(CBMPERM) $(CBMDEV) c $(DEVMAJOR) $(DEVMINOR)
