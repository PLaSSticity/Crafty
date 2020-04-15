# ==============================================================================
#
# Defines.common.mk
#
# ==============================================================================

architecture := $(shell uname -m)

LIBS += -lm

PROG := labyrinth

SRCS += \
	coordinate.c \
	grid.c \
	labyrinth.c \
	maze.c \
	router.c \
	$(LIB)/list.c \
	$(LIB)/mt19937ar.c \
	$(LIB)/pair.c \
	$(LIB)/queue.c \
	$(LIB)/random.c \
	$(LIB)/thread.c \
	$(LIB)/vector.c \
#
OBJS := ${SRCS:.c=.o}

CFLAGS += -DUSE_EARLY_RELEASE

ifeq ($(architecture),ppc64)
 CFLAGS += -DMAP_USE_CONCUREENT_HASHTABLE -DHASHTABLE_SIZE_FIELD -DHASHTABLE_RESIZABLE
 CFLAGS += -DUSE_RBTREE_FOR_FRAGMENT_REASSEMBLE -DRBTREE_SIZE_FIELD
else
 CFLAGS += -DMAP_USE_RBTREE
endif

# ==============================================================================
#
# End of Defines.common.mk
#
# ==============================================================================
