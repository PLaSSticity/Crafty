PROG := tpcc
SRC_DIR := ../src

SRCS += \
	memory.cc \
	pair.cc \
	list.cc \
	hashtable.cc \
	tpcc.cc \
	tpccclient.cc \
	tpccgenerator.cc \
	tpcctables.cc \
	tpccdb.cc \
	clock.cc \
	randomgenerator.cc \
	stupidunit.cc \
	$(SRC_DIR)/mt19937ar.c \
	$(SRC_DIR)/random.c\
	$(SRC_DIR)/thread.c
#
OBJS     := ${SRCS:.cc=.o}
OBJS     := ${OBJS:.c=.o}
OBJS     := ${OBJS:.cpp=.o}

DEFINES  += -std=c++11
CFLAGS   += -I ../include
CPPFLAGS += -I ../include
LIBS     += #-lboost_system
