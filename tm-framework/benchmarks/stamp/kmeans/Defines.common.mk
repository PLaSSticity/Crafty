# ==============================================================================
#
# Defines.common.mk
#
# ==============================================================================

architecture := $(shell uname -m)

ifeq ($(architecture),ppc64)
CFLAGS += -DALIGNED_ALLOC_MEMORY
endif

CFLAGS += -DOUTPUT_TO_STDOUT -g

PROG := kmeans

SRCS += \
	cluster.c \
	common.c \
	kmeans.c \
	normal.c \
	$(LIB)/mt19937ar.c \
	$(LIB)/random.c \
	$(LIB)/thread.c \
#
OBJS := ${SRCS:.c=.o}


# ==============================================================================
#
# End of Defines.common.mk
#
# ==============================================================================
