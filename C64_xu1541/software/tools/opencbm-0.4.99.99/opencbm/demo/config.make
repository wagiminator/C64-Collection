RELATIVEPATH=../../
include ${RELATIVEPATH}LINUX/config.make

CFLAGS     := $(subst ../,../../,$(CFLAGS))
LINK_FLAGS := $(subst ../,../../,$(LINK_FLAGS))
