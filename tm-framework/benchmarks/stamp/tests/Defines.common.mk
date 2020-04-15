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

PROG := tests

SRCS += \
	main.c \
	$(LIB)/bitmap.c \
	$(LIB)/hash.c \
	$(LIB)/queue.c \
	$(LIB)/hashtable.c \
	$(LIB)/rbtree.c \
	$(LIB)/heap.c \
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
