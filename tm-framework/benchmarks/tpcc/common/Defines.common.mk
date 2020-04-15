# ==============================================================================
#
# Defines.common.mk
#
# ==============================================================================


CC       := gcc
CFLAGS   += -g -pthread
CFLAGS   += -O2 -std=c++11
CFLAGS   += -I$(LIB)
CPP      := g++
CPPFLAGS += $(CFLAGS)
LD       := g++
LIBS     += -lpthread

SRCS += \
	memory.cc \
	pair.cc \
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
OBJS := ${SRCS:.cc=.o}

# Remove these files when doing clean
OUTPUT +=

LIB := ../lib

STM := ../../tmf-master/norec



# ==============================================================================
#
# End of Defines.common.mk
#
# ==============================================================================
