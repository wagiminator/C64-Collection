RELATIVEPATH=../
include ../config.make

.PHONY: all mrproper clean install uninstall install-files

DEMO    = flash

PROGS   = $(DEMO)
INC     = $(DEMO).inc

all: $(PROGS)

clean:
	rm -f $(PROGS) *.o *.o65

mrproper: clean
	rm -f $(INC)

$(DEMO).o: $(DEMO).c $(DEMO).inc
	$(CC) $(CFLAGS) -c $(DEMO).c -o $@

$(DEMO): $(DEMO).o
	$(CC) $< -o $@ $(LINK_FLAGS)

install-files:

install:

uninstall:
