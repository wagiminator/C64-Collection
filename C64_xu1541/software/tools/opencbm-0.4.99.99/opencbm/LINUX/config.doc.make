# Find out if we should use linuxdoc or sgml2txt/sgml2latex/sgml2info/sgml2html
#
LINUXDOCTXT = ${shell for c in linuxdoc sgml2txt; do test ! -z `which $$c` && test -f `which $$c` && echo $$c; done | head -n 1}
ifeq "${LINUXDOCTXT}" ""
  $(error You must have linuxdoc or sgmltools installed. Check config.make)
else
 ifeq "${LINUXDOCTXT}" "linuxdoc"
  LINUXDOCLATEX=${LINUXDOCTXT}
  LINUXDOCINFO=${LINUXDOCTXT}
  LINUXDOCHTML=${LINUXDOCTXT}

  LINUXDOCTXTPARAM=-B txt -c latin
  LINUXDOCLATEXPARAM=-B latex -c latin
  LINUXDOCINFOPARAM=-B info -c latin
  LINUXDOCHTMLPARAM=-B html -c latin
 else
  LINUXDOCLATEX=sgml2latex
  LINUXDOCINFO=sgml2info
  LINUXDOCHTML=sgml2html

  LINUXDOCTXTPARAM=-c latin
  LINUXDOCLATEXPARAM=-c latin
  LINUXDOCINFOPARAM=-c latin
  LINUXDOCHTMLPARAM=-c latin
 endif
endif
