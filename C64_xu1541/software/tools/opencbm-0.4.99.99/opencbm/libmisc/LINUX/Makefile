RELATIVEPATH=../
include ${RELATIVEPATH}LINUX/config.make

.PHONY: all clean mrproper install uninstall install-files

CFLAGS += -I../include -I./ $(LIBUSB_CFLAGS)
LDFLAGS += $(LIBUSB_LDFLAGS)

LIB     = libmisc.a
SRCS    = libstring.c configuration.c statedebug.c LINUX/getpluginaddress.c LINUX/dynlibusb.c

OBJS    = $(SRCS:.c=.lo)

all: $(LIB)

clean:
	rm -f $(OBJS) $(LIB)

mrproper: clean

install-files:

install: install-files

uninstall:

.c.o:
	$(CC) $(LIB_CFLAGS) -c -o $@ $<

$(LIB): $(OBJS)
	$(AR) r $@ $(OBJS)
