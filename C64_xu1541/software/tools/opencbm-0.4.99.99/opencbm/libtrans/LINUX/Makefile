RELATIVEPATH=../
include ${RELATIVEPATH}LINUX/config.make

.PHONY: all clean mrproper install uninstall install-files

#CFLAGS += -I../include

LIB     = libtrans.a
SRCS    = pp.c \
	  s1.c \
	  s2.c \
	  turbo.c

INCS    = pp1541.inc \
	  pp1571.inc \
	  s1.inc \
	  s2.inc \
          turbomain.inc

OBJS    = $(SRCS:.c=.lo)

all: $(LIB)

clean:
	rm -f $(OBJS) $(LIB)

mrproper: clean
	rm -f $(INCS)

install-files:

install: install-files

uninstall:

pp1541.inc: pp1541.a65 common.i65

pp1571.inc: pp1571.a65 common.i65

s1.lo: s1.c s1.inc

s2.lo: s2.c s2.inc

pp.lo: pp.c pp1541.inc pp1571.inc

turbo.lo: turbomain.inc

.c.o:
	$(CC) $(LIB_CFLAGS) -c -o $@ $<

$(LIB): $(OBJS)
	$(AR) r $@ $(OBJS)
