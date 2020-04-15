# ==============================================================================
#
# Defines.common.mk
#
# ==============================================================================

architecture := $(shell uname -m)

CFLAGS += -DLIST_NO_DUPLICATES

ifeq ($(architecture),ppc64)
CFLAGS += -DMAP_USE_CONCUREENT_HASHTABLE -DHASHTABLE_SIZE_FIELD -DHASHTABLE_RESIZABLE
else
CFLAGS += -DMAP_USE_RBTREE
endif

PROG := vacation

SRCS += \
	client.c \
	customer.c \
	manager.c \
	reservation.c \
	vacation.c \
	$(LIB)/list.c \
	$(LIB)/pair.c \
	$(LIB)/mt19937ar.c \
	$(LIB)/random.c \
	$(LIB)/rbtree.c \
        $(LIB)/hashtable.c \
        $(LIB)/conc_hashtable.c \
	$(LIB)/thread.c \
        $(LIB)/memory.c
#
OBJS := ${SRCS:.c=.o}


# ==============================================================================
#
# End of Defines.common.mk
#
# ==============================================================================
