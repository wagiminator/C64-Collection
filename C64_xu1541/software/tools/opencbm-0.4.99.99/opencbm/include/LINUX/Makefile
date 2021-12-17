RELATIVEPATH=../
include ${RELATIVEPATH}LINUX/config.make

.PHONY: all clean mrproper install uninstall install-files

INSTHDRS = opencbm.h d64copy.h cbmcopy.h

INSTHDR_INSTALLED=$(foreach t,$(INSTHDRS), $(DESTDIR)/$(INCDIR)/$(t))

all:

clean:

mrproper: clean
	rm -f *~ LINUX/*~

install-files:
	install -d $(DESTDIR)$(INCDIR)
	install -m 644 $(INSTHDRS) $(DESTDIR)$(INCDIR)

install: install-files

uninstall:
	rm -f $(INSTHDR_INSTALLED)
