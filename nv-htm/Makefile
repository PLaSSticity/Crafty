ROOT ?= .
include $(ROOT)/Makefile_common.inc
include $(ROOT)/Makefile_options.inc

BUILD_FOLDER := $(ROOT)/build/

INCLUDES := $(COMMON_INC) \
	$(SOLUTION_INC) 

SRCS     := $(COMMON_SRC) \
	$(SOLUTION_SRC)
#
OBJS     := $(addsuffix .o, $(basename $(SRCS)))

LIB        := libnh.a
BANK       := bank
BANK_SRCS  := $(ROOT)/test/bank.c
BANK_OBJS  := $(addsuffix .o,$(basename $(BANK_SRCS)))
CLOCK      := clock
CLOCK_SRCS := $(ROOT)/test/clock.c
CLOCK_OBJS := $(addsuffix .o,$(basename $(CLOCK_SRCS)))

###########################################
CC       := gcc
CXX      := g++
AR       := ar -rv
CFLAGS   := -g $(OPT) -Wall $(HTM_LIB) $(INCLUDES) $(DEFINES)
CXXFLAGS := -g $(OPT) -std=c++11 $(HTM_LIB) $(INCLUDES) $(DEFINES)
LDFLAGS  := -g -lpthread $(INCLUDES) $(DEFINES) $(COMMON_LIBS)
###########################################

build: $(LIB) # $(BANK) $(CLOCK)

$(OBJS): $(SRCS)

$(LIB): $(OBJS)
ifeq ($(DO_CHECKPOINT),5)
	$(CXX) -g -O3 -c -o $(ROOT)/nvhtm_common/src/log_backward.o $(ROOT)/nvhtm_common/src/log_backward.cpp \
		-std=c++11 $(HTM_LIB) $(INCLUDES) $(DEFINES)
	$(CXX) -g -O1 -c -o $(ROOT)/nvhtm_common/src/log_forward.o $(ROOT)/nvhtm_common/src/log_forward.cpp \
		-std=c++11 $(HTM_LIB) $(INCLUDES) $(DEFINES)
	$(CXX) -g -O3 -c -o $(ROOT)/nvhtm_common/src/log_sorter.o $(ROOT)/nvhtm_common/src/log_sorter.cpp \
		-std=c++11 $(HTM_LIB) $(INCLUDES) $(DEFINES)
endif
ifeq ($(SOLUTION),4)
	$(CXX) -g -O2 -c -o $(ROOT)/nvhtm_common/src/nvhtm_helper.o $(ROOT)/nvhtm_common/src/nvhtm_helper.cpp -std=c++11 $(HTM_LIB) $(INCLUDES) $(DEFINES)
endif
	$(AR) $@ $^

# $(BANK): $(OBJS)
	# $(CXX) -o $@ $(BANK_SRCS) $(OBJS) $(LDFLAGS)

# $(CLOCK): $(OBJS)
	# $(CXX) -o $@ $(CLOCK_SRCS) $(OBJS) $(LDFLAGS)

clean:
	rm -f $(OBJS) $(LIB) $(APP) $(APP_OBJS) `find * -name *.o`
