RELATIVEPATH=../
include ${RELATIVEPATH}LINUX/config.make

.PHONY: all clean mrproper install uninstall install-files

LIBS = libcbm4linux.a
OBJS = cbm.o upload.o
HDRS = cbm4linux.h

all: $(LIBS)

clean:
	rm -f $(LIBS) $(OBJS)

mrproper: clean

install-files:
	install -m 644 $(HDRS) ${DESTDIR}$(INCDIR)
	install -m 644 $(LIBS) ${DESTDIR}$(LIBDIR)

install: install-files

uninstall:
	-cd $(DESTDIR)$(INCDIR) && rm -f $(HDRS)
	-cd $(DESTDIR)$(LIBDIR) && rm -f $(LIBS)

.c.o:
	$(CC) $(LIB_CFLAGS) -DCBM4LINUX_OLDAPI -c $< -o $@

$(LIBS): $(OBJS)
	$(AR) r $@ $(OBJS)
