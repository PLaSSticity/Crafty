# ==============================================================================
#
# Defines.common.mk
#
# ==============================================================================


CC       := gcc
CFLAGS   += -g -pthread
CFLAGS   += -O2 -std=c++11
CFLAGS   += -I ../include
CPP      := g++
CPPFLAGS += $(CFLAGS)
LD       := g++
LIBS     += -lpthread

SRC_DIR  := ../src

SRCS += \
	memory.cc \
	pair.cc \
	data.c \
	learner.c \
	net.c \
	sort.c \
	$(SRC_DIR)/bitmap.c \
	$(SRC_DIR)/list.c \
	$(SRC_DIR)/mt19937ar.c \
	$(SRC_DIR)/queue.c \
	$(SRC_DIR)/random.c \
	$(SRC_DIR)/thread.c \
	$(SRC_DIR)/vector.c \
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
