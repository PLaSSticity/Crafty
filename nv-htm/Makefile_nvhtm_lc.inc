include $(ROOT)/Makefile_nvhtm.inc

SOLUTION_SRC+=$(shell ls -rt -d -1 $(ROOT)/nvhtm_lc/src/*.c $(ROOT)/nvhtm_lc/src/*.cpp)
SOLUTION_INC+=-I $(ROOT)/nvhtm_lc/
