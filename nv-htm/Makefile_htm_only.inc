SOLUTION_SRC:=$(shell ls -rt -d -1 $(ROOT)/htm_only/src/*.c $(ROOT)/htm_only/src/*.cpp)
SOLUTION_INC:=-I $(ROOT)/htm_only/
HTM_ONLY_LIB:=libnh.a
