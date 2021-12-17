RELATIVEPATH=../
include ${RELATIVEPATH}LINUX/config.make

LIBD64COPY=../libd64copy

OBJS = main.o \
 	  $(foreach t,d64copy fs gcr pp s1 s2 std, $(LIBD64COPY)/$(t).o)

PROG = d64copy

CA65_FLAGS += --asm-include-dir ../libd64copy/

EXTRA_A65_INC= \
  $(LIBD64COPY)/warpread1541.inc $(LIBD64COPY)/warpwrite1541.inc \
  $(LIBD64COPY)/warpread1571.inc $(LIBD64COPY)/warpwrite1571.inc \
  $(LIBD64COPY)/turboread1541.inc $(LIBD64COPY)/turbowrite1541.inc \
  $(LIBD64COPY)/turboread1571.inc $(LIBD64COPY)/turbowrite1571.inc \
  $(LIBD64COPY)/pp1541.inc $(LIBD64COPY)/pp1571.inc \
  $(LIBD64COPY)/s1.inc $(LIBD64COPY)/s2.inc

$(LIBD64COPY)/d64copy.o $(LIBD64COPY)/d64copy.lo: \
  $(LIBD64COPY)/d64copy.c $(LIBD64COPY)/d64copy_int.h \
  ../include/opencbm.h ../include/d64copy.h $(LIBD64COPY)/gcr.h \
  $(LIBD64COPY)/warpread1541.inc $(LIBD64COPY)/warpwrite1541.inc \
  $(LIBD64COPY)/warpread1571.inc $(LIBD64COPY)/warpwrite1571.inc \
  $(LIBD64COPY)/turboread1541.inc $(LIBD64COPY)/turbowrite1541.inc \
  $(LIBD64COPY)/turboread1571.inc $(LIBD64COPY)/turbowrite1571.inc
$(LIBD64COPY)/fs.o $(LIBD64COPY)/fs.lo: \
  $(LIBD64COPY)/fs.c $(LIBD64COPY)/d64copy_int.h ../include/opencbm.h \
  ../include/d64copy.h $(LIBD64COPY)/gcr.h
$(LIBD64COPY)/gcr.o $(LIBD64COPY)/gcr.lo: \
  $(LIBD64COPY)/gcr.c $(LIBD64COPY)/gcr.h
$(LIBD64COPY)/pp.o $(LIBD64COPY)/pp.lo: \
  $(LIBD64COPY)/pp.c ../include/opencbm.h $(LIBD64COPY)/d64copy_int.h \
  ../include/d64copy.h $(LIBD64COPY)/gcr.h $(LIBD64COPY)/pp1541.inc \
  $(LIBD64COPY)/pp1571.inc
$(LIBD64COPY)/s1.o $(LIBD64COPY)/s1.lo: \
  $(LIBD64COPY)/s1.c ../include/opencbm.h $(LIBD64COPY)/d64copy_int.h \
  ../include/d64copy.h $(LIBD64COPY)/gcr.h $(LIBD64COPY)/s1.inc
$(LIBD64COPY)/s2.o $(LIBD64COPY)/s2.lo: \
  $(LIBD64COPY)/s2.c ../include/opencbm.h $(LIBD64COPY)/d64copy_int.h \
  ../include/d64copy.h $(LIBD64COPY)/gcr.h $(LIBD64COPY)/s2.inc
$(LIBD64COPY)/std.o $(LIBD64COPY)/std.lo: \
  $(LIBD64COPY)/std.c ../include/opencbm.h \
  $(LIBD64COPY)/d64copy_int.h ../include/d64copy.h $(LIBD64COPY)/gcr.h

include ${RELATIVEPATH}LINUX/prgrules.make
