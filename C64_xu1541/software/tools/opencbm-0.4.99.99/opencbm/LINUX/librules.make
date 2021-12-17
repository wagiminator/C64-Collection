# common rules for shared lib creation
#

# you need to define the following variables to use the rules:
# LIBNAME   name of library
# SRCS      source files for library
# LIBS      link libs for library (optional)

TMPFILE=tempfile.tmp

#
# library naming definitions
#
SHLIB   = $(LIBNAME).$(SHLIB_EXT)
SHLIBV  = $(SHLIB).$(MAJ)
SHLIBV3 = $(SHLIBV).$(MIN).$(REL)
LIB     = $(LIBNAME).a

ifeq "$(OS)" "Darwin"
SHLIBV  = $(LIBNAME).$(MAJ).$(SHLIB_EXT)
SHLIBV3 = $(LIBNAME).$(MAJ).$(MIN).$(REL).$(SHLIB_EXT)
endif

# define object files
OBJS    = $(SRCS:.c=.o)
SHOBJS  = $(SRCS:.c=.lo)

.PHONY: build-lib clean-lib install-lib uninstall-lib update-libcache
.PHONY: install-plugin uninstall-plugin
.PHONY: clean mrproper

# build lib rule
build-lib: $(LIB) $(SHLIB) $(SHLIBV) $(SHLIBV3)

# clean up lib files
clean-lib:
	rm -f $(OBJS) $(SHOBJS) $(LIB) $(SHLIB) $(SHLIBV) $(SHLIBV3)

# install lib
install-lib:
	install -d $(DESTDIR)$(LIBDIR)
	install -m 755 $(SHLIBV3) $(DESTDIR)$(LIBDIR)
	install -m 644 $(LIB) $(DESTDIR)$(LIBDIR)
	cd $(DESTDIR)$(LIBDIR) && ln -sf $(SHLIBV3) $(SHLIBV); ln -sf $(SHLIBV) $(SHLIB)
	# Install the plugin helper tool
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 ${RELATIVEPATH}/LINUX/plugin_helper_tools $(DESTDIR)$(BINDIR)/opencbm_plugin_helper_tools
	# mark that there is no default plugin yet.
	${RELATIVEPATH}/LINUX/plugin_helper_tools setdefaultplugin "$(DESTDIR)$(OPENCBM_CONFIG_PATH)" 00opencbm.conf ""
	
# update lib
update-libcache:
ifeq "$(OS)" "Linux"
	$(shell if test -z "`grep $(LIBDIR) $(DESTDIR)/etc/ld.so.conf`"; then echo $(LIBDIR) >> $(DESTDIR)/etc/ld.so.conf; fi)
ifeq "$(DESTDIR)" ""
	$(LDCONFIG)
endif
endif

# uninstall lib
uninstall-lib:
	cd $(DESTDIR)$(LIBDIR) && rm -f $(LIB) $(SHLIB) $(SHLIBV) $(SHLIBV3)
	$(RELATIVEPATH)/LINUX/plugin_helper_tools uninstall "$(DESTDIR)$(OPENCBM_CONFIG_PATH)" 00opencbm.conf

# install plugin
install-plugin:
	install -d $(DESTDIR)$(PLUGINDIR)
	install -m 755 $(SHLIBV3) $(DESTDIR)$(PLUGINDIR)
	install -m 644 $(LIB) $(DESTDIR)$(PLUGINDIR)
	cd $(DESTDIR)$(PLUGINDIR) && ln -sf $(SHLIBV3) $(SHLIBV); ln -sf $(SHLIBV) $(SHLIB)
	@echo "[${PLUGIN_NAME}]" > ${TMPFILE}
	@echo "location=$(PLUGINDIR)$(SHLIB)" >> ${TMPFILE}
	@echo "" >> ${TMPFILE}
	$(RELATIVEPATH)/LINUX/plugin_helper_tools install "$(DESTDIR)$(OPENCBM_CONFIG_PATH)" 10${PLUGIN_NAME}.conf ${TMPFILE}
	@rm ${TMPFILE}

	# set this plugin as default plugin, if there is none yet.
	$(RELATIVEPATH)/LINUX/plugin_helper_tools setdefaultplugin "$(DESTDIR)$(OPENCBM_CONFIG_PATH)" 00opencbm.conf $(PLUGIN_NAME)

# uninstall plugin
uninstall-plugin:
	-cd $(DESTDIR)$(PLUGINDIR) && rm -f $(LIB) $(SHLIB) $(SHLIBV) $(SHLIBV3)
	-rmdir -p $(DESTDIR)$(PLUGINDIR)
	$(RELATIVEPATH)/LINUX/plugin_helper_tools uninstall "$(DESTDIR)$(OPENCBM_CONFIG_PATH)" 10$(PLUGIN_NAME).conf


# compile rule
.c.o:
	$(CC) $(LIB_CFLAGS) -c -o $@ $<

# link lib
$(SHLIB): $(SHLIBV)
	ln -sf $< $@

$(SHLIBV): $(SHLIBV3)
	ln -sf $< $@

# build shared lib
$(SHLIBV3): $(SHOBJS)
	$(CC) $(LDFLAGS) $(SHLIB_SWITCH) -o $@ $(SONAME)$(SHLIBV) $(SHOBJS) $(LIBS)

# build static lib
$(LIB): $(OBJS)
	$(AR) r $@ $(OBJS)

clean:

mrproper: clean
	rm -f *~ LINUX/*~ WINDOWS/*~
