RELATIVEPATH=../
include ${RELATIVEPATH}LINUX/config.make

LIBD82COPY=../libd82copy

OBJS = $(LIBD82COPY)/fs.o $(LIBD82COPY)/gcr.o $(LIBD82COPY)/std.o $(LIBD82COPY)/d82copy.o main.o
PROG = d82copy
LINKS = 

$(LIBD82COPY)/d82copy.o $(LIBD82COPY)/d82copy.lo: \
  $(LIBD82COPY)/d82copy.c $(LIBD82COPY)/d82copy_int.h \
  ../include/arch.h
$(LIBD82COPY)/fs.o $(LIBD82COPY)/fs.lo: \
  $(LIBD82COPY)/fs.c $(LIBD82COPY)/d82copy_int.h \
  ../include/arch.h
$(LIBD82COPY)/std.o $(LIBD82COPY)/std.lo: \
  $(LIBD82COPY)/std.c ../include/opencbm.h \
  $(LIBD82COPY)/d82copy_int.h

include ${RELATIVEPATH}LINUX/prgrules.make
