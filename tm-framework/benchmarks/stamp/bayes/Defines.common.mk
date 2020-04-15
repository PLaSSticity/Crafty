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

PROG := bayes

SRCS += \
	adtree.c \
	bayes.c \
	data.c \
	learner.c \
	net.c \
	sort.c \
	$(LIB)/bitmap.c \
	$(LIB)/list.c \
	$(LIB)/mt19937ar.c \
	$(LIB)/queue.c \
	$(LIB)/random.c \
	$(LIB)/thread.c \
	$(LIB)/vector.c \
#
OBJS := ${SRCS:.c=.o}

CFLAGS += -DLIST_NO_DUPLICATES
CFLAGS += -DLEARNER_TRY_REMOVE
CFLAGS += -DLEARNER_TRY_REVERSE

# ==============================================================================
#
# End of Defines.common.mk
#
# ==============================================================================
