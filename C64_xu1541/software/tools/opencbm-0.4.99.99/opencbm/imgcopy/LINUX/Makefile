RELATIVEPATH=../
include ${RELATIVEPATH}LINUX/config.make

LIBIMGCOPY=../libimgcopy

OBJS = main.o \
 	  $(foreach t,imgcopy fs pp s1 s2 s3 std, $(LIBIMGCOPY)/$(t).o)

PROG = imgcopy

CA65_FLAGS += --asm-include-dir ../libimgcopy/

EXTRA_A65_INC= \
  $(LIBIMGCOPY)/turboread1541.inc $(LIBIMGCOPY)/turbowrite1541.inc \
  $(LIBIMGCOPY)/turboread1571.inc $(LIBIMGCOPY)/turbowrite1571.inc \
  $(LIBIMGCOPY)/turboread1581.inc $(LIBIMGCOPY)/turbowrite1581.inc \
  $(LIBIMGCOPY)/pp1541.inc $(LIBIMGCOPY)/pp1571.inc \
  $(LIBIMGCOPY)/s1.inc $(LIBIMGCOPY)/s1-1581.inc \
  $(LIBIMGCOPY)/s2.inc $(LIBIMGCOPY)/s2-1581.inc \
  $(LIBIMGCOPY)/s3.inc $(LIBIMGCOPY)/s3-1581.inc

$(LIBIMGCOPY)/imgcopy.o $(LIBIMGCOPY)/imgcopy.lo: \
  $(LIBIMGCOPY)/imgcopy.c $(LIBIMGCOPY)/imgcopy_int.h \
  ../include/opencbm.h ../include/imgcopy.h $(LIBIMGCOPY)/gcr.h \
  $(LIBIMGCOPY)/turboread1541.inc $(LIBIMGCOPY)/turbowrite1541.inc \
  $(LIBIMGCOPY)/turboread1571.inc $(LIBIMGCOPY)/turbowrite1571.inc \
  $(LIBIMGCOPY)/turboread1581.inc $(LIBIMGCOPY)/turbowrite1581.inc
$(LIBIMGCOPY)/fs.o $(LIBIMGCOPY)/fs.lo: \
  $(LIBIMGCOPY)/fs.c $(LIBIMGCOPY)/imgcopy_int.h ../include/opencbm.h \
  ../include/imgcopy.h $(LIBIMGCOPY)/gcr.h
$(LIBIMGCOPY)/pp.o $(LIBIMGCOPY)/pp.lo: \
  $(LIBIMGCOPY)/pp.c ../include/opencbm.h $(LIBIMGCOPY)/imgcopy_int.h \
  ../include/imgcopy.h $(LIBIMGCOPY)/gcr.h $(LIBIMGCOPY)/pp1541.inc \
  $(LIBIMGCOPY)/pp1571.inc
$(LIBIMGCOPY)/s1.o $(LIBIMGCOPY)/s1.lo: \
  $(LIBIMGCOPY)/s1.c ../include/opencbm.h $(LIBIMGCOPY)/imgcopy_int.h \
  ../include/imgcopy.h $(LIBIMGCOPY)/gcr.h $(LIBIMGCOPY)/s1.inc $(LIBIMGCOPY)/s1-1581.inc
$(LIBIMGCOPY)/s2.o $(LIBIMGCOPY)/s2.lo: \
  $(LIBIMGCOPY)/s2.c ../include/opencbm.h $(LIBIMGCOPY)/imgcopy_int.h \
  ../include/imgcopy.h $(LIBIMGCOPY)/gcr.h $(LIBIMGCOPY)/s2.inc $(LIBIMGCOPY)/s2-1581.inc
$(LIBIMGCOPY)/s3.o $(LIBIMGCOPY)/s3.lo: \
  $(LIBIMGCOPY)/s3.c ../include/opencbm.h $(LIBIMGCOPY)/imgcopy_int.h \
  ../include/imgcopy.h $(LIBIMGCOPY)/gcr.h $(LIBIMGCOPY)/s3.inc $(LIBIMGCOPY)/s3-1581.inc
$(LIBIMGCOPY)/std.o $(LIBIMGCOPY)/std.lo: \
  $(LIBIMGCOPY)/std.c ../include/opencbm.h \
  $(LIBIMGCOPY)/imgcopy_int.h ../include/imgcopy.h $(LIBIMGCOPY)/gcr.h

include ${RELATIVEPATH}LINUX/prgrules.make
