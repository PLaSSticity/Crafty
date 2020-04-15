# ==============================================================================
#
# Defines.common.mk
#
# ==============================================================================

architecture := $(shell uname -m)

CFLAGS += -DLIST_NO_DUPLICATES

ifeq ($(architecture),ppc64)
CFLAGS += -DCHUNK_STEP1=2
else
CFLAGS += -DCHUNK_STEP1=12
endif

PROG := genome

SRCS += \
	gene.c \
	genome.c \
	segments.c \
	sequencer.c \
	table.c \
	$(LIB)/bitmap.c \
	$(LIB)/hash.c \
	$(LIB)/hashtable.c \
	$(LIB)/pair.c \
	$(LIB)/random.c \
	$(LIB)/list.c \
	$(LIB)/mt19937ar.c \
	$(LIB)/thread.c \
	$(LIB)/vector.c \
#
OBJS := ${SRCS:.c=.o}


# ==============================================================================
#
# End of Defines.common.mk
#
# ==============================================================================
