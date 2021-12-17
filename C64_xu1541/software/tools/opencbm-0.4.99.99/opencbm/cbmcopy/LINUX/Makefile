RELATIVEPATH=../
include ${RELATIVEPATH}LINUX/config.make

LIBCBMCOPY = ../libcbmcopy

LIBS    = -L$(RELATIVEPATH)/libmisc -lmisc -ldl
CFLAGS := -I$(RELATIVEPATH)/libcbmcopy $(CFLAGS)

OBJS = main.o pc64.o t64.o raw.o \
 	  $(foreach t,cbmcopy pp s1 s2 std, $(LIBCBMCOPY)/$(t).o)

EXTRA_A65_INC= \
  $(LIBCBMCOPY)/turboread1541.inc $(LIBCBMCOPY)/turboread1571.inc \
  $(LIBCBMCOPY)/turboread1581.inc $(LIBCBMCOPY)/turbowrite1541.inc \
  $(LIBCBMCOPY)/turbowrite1571.inc $(LIBCBMCOPY)/turbowrite1581.inc \
  $(LIBCBMCOPY)/ppr-1541.inc $(LIBCBMCOPY)/ppr-1571.inc \
  $(LIBCBMCOPY)/ppw-1541.inc $(LIBCBMCOPY)/ppw-1571.inc \
  $(LIBCBMCOPY)/s1r.inc $(LIBCBMCOPY)/s1w.inc $(LIBCBMCOPY)/s1r-1581.inc \
  $(LIBCBMCOPY)/s1w-1581.inc \
  $(LIBCBMCOPY)/s2r.inc $(LIBCBMCOPY)/s2w.inc $(LIBCBMCOPY)/s2r-1581.inc \
  $(LIBCBMCOPY)/s2w-1581.inc

PROG = cbmcopy
LINKS = cbmread cbmwrite

$(LIBCBMCOPY)/cbmcopy.o $(LIBCBMCOPY)/cbmcopy.lo: \
  $(LIBCBMCOPY)/cbmcopy.c ../include/opencbm.h \
  ../include/cbmcopy.h $(LIBCBMCOPY)/cbmcopy_int.h \
  $(LIBCBMCOPY)/turboread1541.inc $(LIBCBMCOPY)/turboread1571.inc \
  $(LIBCBMCOPY)/turboread1581.inc $(LIBCBMCOPY)/turbowrite1541.inc \
  $(LIBCBMCOPY)/turbowrite1571.inc $(LIBCBMCOPY)/turbowrite1581.inc
$(LIBCBMCOPY)/pp.o $(LIBCBMCOPY)/pp.lo: \
  $(LIBCBMCOPY)/pp.c ../include/opencbm.h $(LIBCBMCOPY)/cbmcopy_int.h \
  $(LIBCBMCOPY)/ppr-1541.inc $(LIBCBMCOPY)/ppr-1571.inc \
  $(LIBCBMCOPY)/ppw-1541.inc $(LIBCBMCOPY)/ppw-1571.inc
$(LIBCBMCOPY)/s1.o $(LIBCBMCOPY)/s1.lo: \
  $(LIBCBMCOPY)/s1.c ../include/opencbm.h $(LIBCBMCOPY)/cbmcopy_int.h \
  $(LIBCBMCOPY)/s1r.inc $(LIBCBMCOPY)/s1w.inc $(LIBCBMCOPY)/s1r-1581.inc \
  $(LIBCBMCOPY)/s1w-1581.inc
$(LIBCBMCOPY)/s2.o $(LIBCBMCOPY)/s2.lo: \
  $(LIBCBMCOPY)/s2.c ../include/opencbm.h $(LIBCBMCOPY)/cbmcopy_int.h \
  $(LIBCBMCOPY)/s2r.inc $(LIBCBMCOPY)/s2w.inc $(LIBCBMCOPY)/s2r-1581.inc \
  $(LIBCBMCOPY)/s2w-1581.inc

include ${RELATIVEPATH}LINUX/prgrules.make
