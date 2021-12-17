RELATIVEPATH=../
include ${RELATIVEPATH}LINUX/config.make
ifneq "${CREATELINUXDOC}" ""
include ${RELATIVEPATH}LINUX/config.doc.make
endif

.PHONY: all clean mrproper install uninstall install-files doc install-files-doc install--doc

SGML = opencbm.sgml

PDF  = opencbm.pdf
TXT  = opencbm.txt
WINTXT  = opencbm.win.txt
TEX  = opencbm.tex
DVI  = opencbm.dvi
INFO = opencbm.info
HTML = html/opencbm.html

EPSFILES  = zadig-uac.eps zadig-listalldevices.eps zadig-xum1541.eps zadig-xu1541.eps zadig-success.eps opencbm-software-installed.eps opencbm-software-installed-uninstall.eps

EPS  = $(EPSFILES)
PNG  = $(addsuffix .png, $(basename $(EPSFILES)))

%.win.txt: %.txt
	sed -e 's/.//g;' -e 's/\[[0-9]*m//g;' $< > $@
	unix2dos $@

%.txt: %.sgml
ifeq "${CREATELINUXDOC}" ""
	CREATELINUXDOC=1 ${MAKE} --silent -f LINUX/Makefile $@
else
	${LINUXDOCTXT} ${LINUXDOCTXTPARAM} $<
endif

%.eps: figures/%.eps.gz
	gzip -d -c $< > $@

%.png: figures/%.png
	cp -p $< $@

%.pdf: %.ps
	ps2pdf $<

%.ps: %.dvi
	dvips -o $*.ps $<

%.dvi: %.sgml ${EPS} ${PNG}
ifeq "${CREATELINUXDOC}" ""
	CREATELINUXDOC=1 ${MAKE} --silent -f LINUX/Makefile $@
else
	${LINUXDOCLATEX} ${LINUXDOCLATEXPARAM} -o dvi $<
endif

%.info: %.sgml
ifeq "${CREATELINUXDOC}" ""
	CREATELINUXDOC=1 ${MAKE} --silent -f LINUX/Makefile $@
else
	${LINUXDOCINFO} ${LINUXDOCINFOPARAM} $<
endif
	sed -i -e 's/^\([ \t]*\\input texinfo\)/INFO-DIR-SECTION Miscellaneous\nSTART-INFO-DIR-ENTRY\n* opencbm: (opencbm).	Commodore serial IEC bus support.\nEND-INFO-DIR-ENTRY\n\1/' $@

%.html: opencbm.sgml
	([ ! -d html ] && mkdir html/) || true
ifeq "${CREATELINUXDOC}" ""
	CREATELINUXDOC=1 ${MAKE} --silent -f LINUX/Makefile $@
else
	cd html; ${LINUXDOCHTML} ${LINUXDOCHTMLPARAM} -s 2 ../$<; cd ..
	cp -p ${PNG} html/
endif

all: $(TXT) $(DVI) $(INFO) $(HTML) ${PDF}

doc: all

install-doc: install

install-files-doc: install-files

clean:
	rm -f $(TEX)

mrproper: clean
	rm -f $(DVI) $(INFO) $(TXT) ${PDF} ${WINTXT} html/* || true
	([ -d html/ ] && rmdir html/) || true
	rm -f *~ LINUX/*~ WINDOWS/*~

install-files:
	install -d $(DESTDIR)$(INFODIR)
	gzip -c9 $(INFO) > $(DESTDIR)$(INFODIR)/$(INFO).gz

install: install-files
	test -z `type -p mkinfodir` || mkinfodir || /bin/true

uninstall:
	rm -f $(DESTDIR)$(INFODIR)/$(INFO).gz
