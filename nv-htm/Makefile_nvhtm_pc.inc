include $(ROOT)/Makefile_nvhtm.inc

SOLUTION_SRC+=$(shell ls -rt -d -1 $(ROOT)/nvhtm_pc/src/*.c $(ROOT)/nvhtm_pc/src/*.cpp)
SOLUTION_INC+=-I $(ROOT)/nvhtm_pc/
