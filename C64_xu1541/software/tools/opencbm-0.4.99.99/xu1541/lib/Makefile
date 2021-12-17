include ../include/common.mk

CFLAGS+= $(CFLAGS_EXTRA) -g -Wall -pedantic -I../include/
LDFLAGS+=$(LDFLAGS_EXTRA)

#LIB_SUFFIX=.a

LIB = libxu1541$(LIB_WIN).a

SRCS = \
	close.c \
	display_device_info.c \
	find.c \
	get_device_info.c \
	get_pagesize.c \
	is_in_bootloader_mode.c \
	set_to_boot_mode.c \
	wait.c \

OBJS = $(SRCS:.c=.o)

.INTERMEDIATE: $(OBJS)

all: $(LIB)

clean:
	rm -f *.o *~

mrproper: clean
	rm -f $(LIB)

$(LIB): $(OBJS)
	$(AR) -r $@ $(OBJS)
