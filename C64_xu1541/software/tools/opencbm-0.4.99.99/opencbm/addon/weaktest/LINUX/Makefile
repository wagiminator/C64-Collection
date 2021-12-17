RELATIVEPATH=../../
include ${RELATIVEPATH}LINUX/config.make
CFLAGS     := $(subst ../,../../,$(CFLAGS))
LINK_FLAGS := $(subst ../,../../,$(LINK_FLAGS))

CFLAGS     += -I../../libd64copy

OBJS = main.o
PROG = weaktest
MAN1 =

include ${RELATIVEPATH}LINUX/prgrules.make
